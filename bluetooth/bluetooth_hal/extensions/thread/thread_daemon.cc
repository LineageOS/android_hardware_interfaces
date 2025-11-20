/*
 * Copyright 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "bthal.thread_dispatcher.daemon"

#include "bluetooth_hal/extensions/thread/thread_daemon.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "android-base/logging.h"
#include "android-base/properties.h"
#include "android-base/stringprintf.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/extensions/thread/socket_processor.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/system_call_wrapper.h"

namespace bluetooth_hal {
namespace thread {

using ::bluetooth_hal::Property;
using ::bluetooth_hal::debug::DebugCentral;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciConstants;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::util::SystemCallWrapper;

void ThreadDaemon::SendUplink(const HalPacket& packet) {
  std::lock_guard<std::mutex> guard(client_mtx_);

  if (!is_daemon_running_) {
    LOG(WARNING) << __func__ << ": Daemon is not running.";
    return;
  }

  if (!is_client_connected_) {
    LOG(WARNING) << __func__ << ": Thread HAL is not connected.";
    return;
  }

  if (packet.empty()) {
    LOG(WARNING) << __func__ << ": Data is empty.";
    return;
  }

  std::vector<uint8_t> spinel_packet = ExtractFromHalPacket(packet);
  socket_processor_->Send(spinel_packet);
}

void ThreadDaemon::SendDownlink(const std::vector<uint8_t>& packet) {
  if (!CheckIfHardwareReset(packet)) {
    HalPacket vendor_packet = ConstructToHalPacket(packet);
    (*hal_packet_cb_)(vendor_packet);
    return;
  }

  // Handle hardware reset if a specific packet is received.
  ANCHOR_LOG_WARNING(AnchorType::THREAD_HARDWARE_RESET)
      << __func__ << ": Hardware reset from Thread HAL.";
  SocketProcessor::Cleanup();
  SystemCallWrapper::GetWrapper().Kill(getpid(), SIGKILL);
}

bool ThreadDaemon::IsDaemonRunning() const { return is_daemon_running_; }

bool ThreadDaemon::Start() {
  LOG(INFO) << __func__;

  if (std::atomic_exchange(&is_daemon_running_, true)) {
    LOG(WARNING)
        << __func__
        << ": Daemon is already started. Close it first before restarting.";
    return false;
  }

  require_starting_ = true;
  if (!StartDaemon()) {
    LOG(ERROR) << __func__ << ": Failed to start the daemon.";
    is_daemon_running_ = false;
    require_starting_ = false;
    return false;
  }

  return true;
}

bool ThreadDaemon::Stop() {
  LOG(INFO) << __func__;

  if (!std::atomic_exchange(&is_daemon_running_, false)) {
    LOG(WARNING) << __func__
                 << ": Daemon is already stopped. No need to close.";
    return false;
  }

  StopDaemon();
  require_starting_ = false;

  return true;
}

void ThreadDaemon::ConfigureSocketProcessor() {
  SocketProcessor::Initialize(
      kThreadDispatcherSocketPath,
      [this](const std::vector<uint8_t>& data) { SendDownlink(data); });

  socket_processor_ = SocketProcessor::GetProcessor();

  auto socket_mode = static_cast<SocketMode>(::android::base::GetIntProperty(
      Property::kThreadDispatcherSocketMode,
      static_cast<int>(SocketMode::kSockModeSeqPacket),
      static_cast<int>(SocketMode::kSockModeStream),
      static_cast<int>(SocketMode::kSockModeSeqPacket)));

  LOG(INFO) << __func__ << ": socket mode: " << static_cast<int>(socket_mode);
  socket_processor_->SetSocketMode(socket_mode);
}

bool ThreadDaemon::StartDaemon() {
  LOG(INFO) << __func__;

  int pipe_fds[2];
  if (SystemCallWrapper::GetWrapper().CreatePipe(pipe_fds, O_NONBLOCK)) {
    LOG(ERROR) << __func__ << ": Failed to create pipe.";
    return false;
  }

  notification_listen_fd_ = pipe_fds[0];
  notification_write_fd_ = pipe_fds[1];

  server_thread_ = std::thread(&ThreadDaemon::DaemonRoutine, this);

  if (!server_thread_.joinable()) {
    LOG(ERROR) << __func__ << ": Server thread is not joinable.";
    return false;
  }

  return true;
}

bool ThreadDaemon::StopDaemon() {
  LOG(INFO) << __func__;

  NotifyDaemonToStop();

  if (server_thread_.joinable() &&
      std::this_thread::get_id() != server_thread_.get_id()) {
    server_thread_.join();
  }

  SystemCallWrapper::GetWrapper().Close(notification_listen_fd_);
  SystemCallWrapper::GetWrapper().Close(notification_write_fd_);
  notification_listen_fd_ = kInvalidFileDescriptor;
  notification_write_fd_ = kInvalidFileDescriptor;

  return true;
}

bool ThreadDaemon::NotifyDaemonToStop() {
  LOG(INFO) << __func__;

  uint8_t stub_buffer = 0;  // Notification buffer.

  if (TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Write(
          notification_write_fd_, &stub_buffer, sizeof(stub_buffer))) < 0) {
    LOG(ERROR) << __func__ << ": Failed to write to notification pipe.";
    return false;
  }

  return true;
}

bool ThreadDaemon::AcceptClient() {
  DURATION_TRACKER(AnchorType::THREAD_ACCEPT_CLIENT,
                   ::android::base::StringPrintf("Accept Thread client"));
  LOG(DEBUG) << __func__ << ": Start processing connect request from client.";

  int new_client_socket = socket_processor_->AcceptClient();

  if (new_client_socket == kInvalidFileDescriptor) {
    LOG(WARNING) << __func__ << ": Unable to accept client.";
    return false;
  }

  if (socket_processor_->GetClientSocket() != kInvalidFileDescriptor ||
      std::atomic_exchange(&is_client_connected_, true)) {
    SystemCallWrapper::GetWrapper().Close(new_client_socket);
    LOG(WARNING) << __func__ << ": Already connected to another client.";
    return false;
  }

  std::lock_guard<std::mutex> guard(client_mtx_);
  socket_processor_->SetClientSocket(new_client_socket);
  LOG(INFO) << __func__ << ": Successfully accepted new client.";

  return true;
}

void ThreadDaemon::MonitorSocket() {
  LOG(DEBUG) << __func__
             << ": Server socket: " << socket_processor_->GetServerSocket();

  while (is_daemon_running_) {
    fd_set monitor_fds;
    PrepareFdsForMonitor(&monitor_fds);

    LOG(DEBUG) << __func__ << ": Daemon is idle...";

    int ret_val = SystemCallWrapper::GetWrapper().Select(
        FD_SETSIZE, &monitor_fds, nullptr, nullptr, nullptr);
    if (ret_val <= 0) {
      continue;  // No activity or an error occurred, continue monitoring.
    }

    if (SystemCallWrapper::GetWrapper().FdIsSet(notification_listen_fd_,
                                                &monitor_fds)) {
      ANCHOR_LOG(AnchorType::THREAD_DADEMON_CLOSED)
          << __func__ << ": Daemon is terminated by notification...";
      LOG(DEBUG) << __func__ << ": Daemon is terminated by notification...";
      uint8_t stub_buffer = 0;  // Reading one byte to clear the notification.
      TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Read(
          notification_listen_fd_, &stub_buffer, sizeof(stub_buffer)));
      continue;
    }

    if (SystemCallWrapper::GetWrapper().FdIsSet(
            socket_processor_->GetSocketFileMonitor(), &monitor_fds)) {
      constexpr size_t kBufferSize = sizeof(inotify_event) + NAME_MAX + 1;
      char buffer[kBufferSize] = {0};
      ssize_t bytes_read = SystemCallWrapper::GetWrapper().Read(
          socket_processor_->GetSocketFileMonitor(), &buffer, sizeof(buffer));

      if (bytes_read > 0) {
        inotify_event* event = reinterpret_cast<inotify_event*>(buffer);
        if (event->mask & IN_DELETE &&
            !socket_processor_->IsSocketFileExisted()) {
          ANCHOR_LOG_DEBUG(AnchorType::THREAD_SOCKET_FILE_DELETED)
              << __func__ << ": Socket file is deleted, need to restart...";
          socket_processor_->CloseSocketFileMonitor();
          require_starting_ = true;
          break;
        }
      }
    }

    if (is_client_connected_ &&
        SystemCallWrapper::GetWrapper().FdIsSet(
            socket_processor_->GetClientSocket(), &monitor_fds)) {
      if (!socket_processor_->Recv()) {
        ANCHOR_LOG_ERROR(AnchorType::THREAD_CLIENT_ERROR)
            << __func__ << ": Daemon receives from client failed...";
        CleanUpClient();
      }
    }

    if (SystemCallWrapper::GetWrapper().FdIsSet(
            socket_processor_->GetServerSocket(), &monitor_fds)) {
      ANCHOR_LOG_DEBUG(AnchorType::THREAD_CLIENT_CONNECT)
          << __func__ << ": Daemon receives client connect request...";
      AcceptClient();
    }
  }
}

void ThreadDaemon::DaemonRoutine() {
  while (require_starting_) {
    require_starting_ = false;
    LOG(INFO) << __func__ << ": Daemon is open.";

    if (socket_processor_->OpenServer()) {
      if (socket_processor_->OpenSocketFileMonitor() ==
          kInvalidFileDescriptor) {
        LOG(WARNING) << __func__ << ": Unable to monitor socket file.";
      }
      MonitorSocket();
    }

    LOG(INFO) << __func__ << ": Daemon is closed.";

    // Release resources.
    CleanUpClient();
    CleanUpServer();
    socket_processor_->CloseSocketFileMonitor();
  }
}

void ThreadDaemon::CleanUpServer() { socket_processor_->CloseServer(); }

void ThreadDaemon::CleanUpClient() {
  std::lock_guard<std::mutex> guard(client_mtx_);
  is_client_connected_ = false;
  socket_processor_->CloseClient();
}

bool ThreadDaemon::CheckIfHardwareReset(const std::vector<uint8_t>& packet) {
  return packet.size() == kHardwareResetCommandSize &&
         packet[0] == kSpinelHeader && packet[1] == kThreadCommandReset &&
         packet[2] == kThreadCommandResetHardware;
}

void ThreadDaemon::PrepareFdsForMonitor(fd_set* monitor_fds) {
  // Ensure the daemon thread will not become a zombie.
  CHECK_NE(notification_listen_fd_, kInvalidFileDescriptor);

  int server_fd = socket_processor_->GetServerSocket();
  int client_fd = socket_processor_->GetClientSocket();
  int monitor_fd = socket_processor_->GetSocketFileMonitor();

  SystemCallWrapper::GetWrapper().FdZero(monitor_fds);

  if (is_daemon_running_ && server_fd != kInvalidFileDescriptor) {
    SystemCallWrapper::GetWrapper().FdSet(server_fd, monitor_fds);
  }

  if (is_client_connected_ && client_fd != kInvalidFileDescriptor) {
    SystemCallWrapper::GetWrapper().FdSet(client_fd, monitor_fds);
  }

  if (monitor_fd != kInvalidFileDescriptor) {
    SystemCallWrapper::GetWrapper().FdSet(monitor_fd, monitor_fds);
  }

  SystemCallWrapper::GetWrapper().FdSet(notification_listen_fd_, monitor_fds);
}

HalPacket ThreadDaemon::ConstructToHalPacket(
    const std::vector<uint8_t>& packet) {
  HalPacket hal_packet;

  hal_packet.push_back(static_cast<uint8_t>(HciPacketType::kThreadData));

  // Reserve two bytes for packet size.
  hal_packet.push_back(0);  // Placeholder for first byte.
  hal_packet.push_back(0);  // Placeholder for second byte.

  uint16_t packet_size = static_cast<uint16_t>(packet.size());
  hal_packet.push_back(static_cast<uint8_t>(packet_size & 0xFF));
  hal_packet.push_back(static_cast<uint8_t>((packet_size >> 8) & 0xFF));

  // Append the original packet data.
  hal_packet.insert(hal_packet.end(), packet.begin(), packet.end());

  return hal_packet;
}

std::vector<uint8_t> ThreadDaemon::ExtractFromHalPacket(
    const HalPacket& packet) {
  std::vector<uint8_t> raw_packet;
  if (packet.size() < 1 + HciConstants::kHciThreadPreambleSize) {
    LOG(WARNING) << __func__ << ": Invalid vendor data format.";
    return raw_packet;
  }

  if (packet.GetType() != HciPacketType::kThreadData) {
    return raw_packet;
  }

  uint16_t packet_size = static_cast<uint16_t>(packet[3]) |
                         (static_cast<uint16_t>(packet[4]) << 8);

  if (packet.size() != 1 + HciConstants::kHciThreadPreambleSize + packet_size) {
    LOG(WARNING) << __func__
                 << ": Data size does not match with the actual data.";
    return raw_packet;
  }

  // Extract the raw packet data.
  raw_packet.insert(raw_packet.end(),
                    packet.begin() + 1 + HciConstants::kHciThreadPreambleSize,
                    packet.end());

  return raw_packet;
}

}  // namespace thread
}  // namespace bluetooth_hal
