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

#define LOG_TAG "bthal.thread_dispatcher.socket"

#include "bluetooth_hal/extensions/thread/socket_processor.h"

#include <grp.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "android-base/logging.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/util/system_call_wrapper.h"

namespace bluetooth_hal {
namespace thread {

using ::bluetooth_hal::hci::HalPacketCallback;
using ::bluetooth_hal::hci::HciConstants;
using ::bluetooth_hal::util::SystemCallWrapper;

enum class SocketDirection {
  kSend = 0,
  kRecv,
};

enum class ReadState {
  kDataHeader,
  kDataFlag,
  kDataPayload,
};

class SocketProcessorImpl;
SocketProcessorImpl* processor = nullptr;

class SocketProcessorImpl : public SocketProcessor {
 public:
  SocketProcessorImpl(const std::string& socket_path,
                      std::optional<HalPacketCallback> hal_packet_cb);

  ~SocketProcessorImpl() override;

  bool Send(const std::vector<uint8_t>& data) override;

  bool Recv() override;

  bool OpenServer() override;

  void CloseServer() override;

  void CloseClient() override;

  int AcceptClient() override;

  void SetServerSocket(int server_socket) override;

  void SetClientSocket(int client_socket) override;

  void SetSocketMode(SocketMode socket_mode) override;

  int GetServerSocket() const override;

  int GetClientSocket() const override;

  bool IsSocketFileExisted() const override;

  int OpenSocketFileMonitor() override;

  void CloseSocketFileMonitor() override;

  int GetSocketFileMonitor() override;

 private:
  static constexpr int kMaxWaitingConnectReq = 3;

  bool SendPacket(const std::vector<uint8_t>& data);

  bool RecvPacket();

  bool SendStream(const std::vector<uint8_t>& data);

  bool RecvStream();

  bool RecvStreamWithFixLength(uint16_t length);

  void ResetReadState();

  bool CreateSocket();

  bool BindSocket();

  bool ListenForClients();

  void PrintSocketErr(int ret_val, SocketDirection dir);

  ReadState read_state_{ReadState::kDataHeader};
  SocketMode socket_mode_ = SocketMode::kSockModeSeqPacket;
  size_t payload_length_{0};
  std::vector<uint8_t> packet_;
  std::vector<uint8_t> data_;
  int server_socket_;
  int client_socket_;
  int socket_file_monitor_fd_;
  std::string socket_path_;
  std::optional<HalPacketCallback> hal_packet_cb_;
};

SocketProcessorImpl::SocketProcessorImpl(
    const std::string& socket_path,
    std::optional<HalPacketCallback> hal_packet_cb)
    : server_socket_(kInvalidFileDescriptor),
      client_socket_(kInvalidFileDescriptor),
      socket_file_monitor_fd_(kInvalidFileDescriptor),
      socket_path_(socket_path),
      hal_packet_cb_(hal_packet_cb) {}

SocketProcessorImpl::~SocketProcessorImpl() {
  SystemCallWrapper::GetWrapper().Unlink(socket_path_.c_str());
}

bool SocketProcessorImpl::Send(const std::vector<uint8_t>& data) {
  LOG(DEBUG) << __func__ << ": Sending packet to client.";

  if (socket_mode_ == SocketMode::kSockModeSeqPacket) {
    return SendPacket(data);
  }
  if (socket_mode_ == SocketMode::kSockModeStream) {
    return SendStream(data);
  }
  return false;
}

bool SocketProcessorImpl::Recv() {
  LOG(DEBUG) << __func__ << ": Receiving packet from client.";

  if (socket_mode_ == SocketMode::kSockModeSeqPacket) {
    return RecvPacket();
  }
  if (socket_mode_ == SocketMode::kSockModeStream) {
    return RecvStream();
  }
  return false;
}

bool SocketProcessorImpl::OpenServer() {
  return CreateSocket() && BindSocket() && ListenForClients();
}

void SocketProcessorImpl::CloseServer() {
  SystemCallWrapper::GetWrapper().Close(server_socket_);
  SystemCallWrapper::GetWrapper().Unlink(socket_path_.c_str());
  server_socket_ = kInvalidFileDescriptor;
}

void SocketProcessorImpl::CloseClient() {
  SystemCallWrapper::GetWrapper().Close(client_socket_);
  client_socket_ = kInvalidFileDescriptor;
}

int SocketProcessorImpl::AcceptClient() {
  sockaddr_un client_address;
  socklen_t client_address_len = sizeof(client_address);

  if (server_socket_ == kInvalidFileDescriptor) {
    return kInvalidFileDescriptor;
  }

  int new_client_socket = SystemCallWrapper::GetWrapper().Accept(
      server_socket_, reinterpret_cast<struct sockaddr*>(&client_address),
      &client_address_len);

  return new_client_socket;
}

void SocketProcessorImpl::SetServerSocket(int server_socket) {
  server_socket_ = server_socket;
}

void SocketProcessorImpl::SetClientSocket(int client_socket) {
  client_socket_ = client_socket;
}

void SocketProcessorImpl::SetSocketMode(SocketMode socket_mode) {
  if (server_socket_ != kInvalidFileDescriptor) {
    return;
  }

  if (socket_mode != SocketMode::kSockModeSeqPacket &&
      socket_mode != SocketMode::kSockModeStream) {
    return;
  }

  socket_mode_ = socket_mode;
}

int SocketProcessorImpl::GetServerSocket() const { return server_socket_; }

int SocketProcessorImpl::GetClientSocket() const { return client_socket_; }

bool SocketProcessorImpl::IsSocketFileExisted() const {
  struct stat st;
  return (SystemCallWrapper::GetWrapper().Stat(socket_path_.c_str(), &st) ==
          0) &&
         SystemCallWrapper::GetWrapper().IsSocketFile(st.st_mode);
}

int SocketProcessorImpl::OpenSocketFileMonitor() {
  if (socket_file_monitor_fd_ != kInvalidFileDescriptor) {
    return socket_file_monitor_fd_;
  }

  socket_file_monitor_fd_ = SystemCallWrapper::GetWrapper().InotifyInit();
  if (socket_file_monitor_fd_ == kInvalidFileDescriptor) {
    LOG(WARNING) << __func__ << ": Error creating inotify processor.";
    return socket_file_monitor_fd_;
  }

  int watch_fd = SystemCallWrapper::GetWrapper().InotifyAddWatch(
      socket_file_monitor_fd_, kThreadDispatcherFolderPath, IN_DELETE);
  if (watch_fd == kInvalidFileDescriptor) {
    LOG(WARNING) << __func__ << ": Error adding watch to socket file.";
    socket_file_monitor_fd_ = kInvalidFileDescriptor;
    return socket_file_monitor_fd_;
  }

  return socket_file_monitor_fd_;
}

void SocketProcessorImpl::CloseSocketFileMonitor() {
  SystemCallWrapper::GetWrapper().Close(socket_file_monitor_fd_);
  socket_file_monitor_fd_ = kInvalidFileDescriptor;
}

int SocketProcessorImpl::GetSocketFileMonitor() {
  return socket_file_monitor_fd_;
}

bool SocketProcessorImpl::SendPacket(const std::vector<uint8_t>& data) {
  ssize_t bytes_sent = TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Send(
      client_socket_, data.data(), data.size(), 0));
  if (bytes_sent <= 0) {
    PrintSocketErr(bytes_sent, SocketDirection::kSend);
    return false;
  }
  return true;
}

bool SocketProcessorImpl::RecvPacket() {
  packet_.resize(kRadioSpinelRxFrameBufferSize);
  ssize_t bytes_read = TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Recv(
      client_socket_, packet_.data(), packet_.size(), 0));
  if (bytes_read <= 0) {
    PrintSocketErr(bytes_read, SocketDirection::kRecv);
    return false;
  }

  packet_.resize(bytes_read);
  (*hal_packet_cb_)(packet_);
  return true;
}

bool SocketProcessorImpl::SendStream(const std::vector<uint8_t>& data) {
  uint8_t head_buffer[1] = {kSocketSpecificHeader};
  ssize_t bytes_sent = TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Send(
      client_socket_, head_buffer, sizeof(head_buffer), 0));
  if (bytes_sent <= 0) {
    PrintSocketErr(bytes_sent, SocketDirection::kSend);
    return false;
  }

  uint16_t packet_size = static_cast<uint16_t>(data.size());
  uint8_t len_buffer[2] = {static_cast<uint8_t>(packet_size & 0xFF),
                           static_cast<uint8_t>((packet_size >> 8) & 0xFF)};
  bytes_sent = TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Send(
      client_socket_, len_buffer, sizeof(len_buffer), 0));
  if (bytes_sent <= 0) {
    PrintSocketErr(bytes_sent, SocketDirection::kSend);
    return false;
  }

  size_t total_bytes_sent = 0;
  while (total_bytes_sent < data.size()) {
    ssize_t bytes_sent =
        TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Send(
            client_socket_, data.data() + total_bytes_sent,
            data.size() - total_bytes_sent, 0));
    if (bytes_sent <= 0) {
      PrintSocketErr(bytes_sent, SocketDirection::kSend);
      return false;
    }
    total_bytes_sent += bytes_sent;
  }
  return true;
}

bool SocketProcessorImpl::RecvStream() {
  size_t read_len = 0;

  switch (read_state_) {
    case ReadState::kDataHeader:
      read_len = 1;  // Header size is 1.
      break;
    case ReadState::kDataFlag:
      read_len = 2;  // Length size is 2.
      break;
    case ReadState::kDataPayload:
      read_len = payload_length_;
      break;
    default:
      LOG(ERROR) << __func__ << ": Invalid read state.";
      return false;
  }

  if (!RecvStreamWithFixLength(read_len)) {
    return false;
  }

  switch (read_state_) {
    case ReadState::kDataHeader: {
      if (data_[0] != kSocketSpecificHeader) {
        LOG(ERROR) << __func__ << ": Invalid header type.";
        ResetReadState();
        return false;
      }
      read_state_ = ReadState::kDataFlag;
      break;
    }

    case ReadState::kDataFlag: {
      uint16_t payload_size = static_cast<uint16_t>(data_[0]) |
                              (static_cast<uint16_t>(data_[1]) << 8);
      if (!payload_size) {
        LOG(ERROR) << __func__ << ": Invalid payload size.";
        ResetReadState();
        return false;
      }
      packet_.resize(payload_size);
      payload_length_ = payload_size;
      read_state_ = ReadState::kDataPayload;
      break;
    }

    case ReadState::kDataPayload: {
      memcpy(packet_.data(), data_.data(), payload_length_);
      (*hal_packet_cb_)(packet_);
      ResetReadState();
      break;
    }
  }

  return true;
}

bool SocketProcessorImpl::RecvStreamWithFixLength(uint16_t length) {
  if (!length) {
    return false;
  }

  data_.resize(length);
  size_t total_bytes_should_read = length;
  ssize_t cur_idx = 0;

  while (total_bytes_should_read > 0) {
    ssize_t bytes_read =
        TEMP_FAILURE_RETRY(SystemCallWrapper::GetWrapper().Recv(
            client_socket_, data_.data() + cur_idx, total_bytes_should_read,
            0));
    if (bytes_read <= 0) {
      PrintSocketErr(bytes_read, SocketDirection::kRecv);
      return false;
    }
    cur_idx += bytes_read;
    total_bytes_should_read -= bytes_read;
  }

  return true;
}

void SocketProcessorImpl::ResetReadState() {
  read_state_ = ReadState::kDataHeader;
  payload_length_ = 0;
}

bool SocketProcessorImpl::CreateSocket() {
  int new_server_socket = SystemCallWrapper::GetWrapper().Socket(
      AF_UNIX, static_cast<int>(socket_mode_), 0);

  if (new_server_socket == kInvalidFileDescriptor) {
    LOG(ERROR) << __func__ << ": Unable to create the socket.";
    return false;
  }

  server_socket_ = new_server_socket;
  return true;
}

bool SocketProcessorImpl::BindSocket() {
  SystemCallWrapper::GetWrapper().Unlink(socket_path_.c_str());
  sockaddr_un server_address;
  server_address.sun_family = AF_UNIX;
  strcpy(server_address.sun_path, socket_path_.c_str());

  if (SystemCallWrapper::GetWrapper().Bind(
          server_socket_, reinterpret_cast<struct sockaddr*>(&server_address),
          sizeof(server_address)) == -1) {
    LOG(ERROR) << __func__ << ": Unable to bind the socket.";
    CloseServer();
    return false;
  }

  // Change the permission of the socket file.
  const char* group_name = "system";
  struct group* grp_info = getgrnam(group_name);
  if (grp_info != nullptr) {
    chown(socket_path_.c_str(), -1, grp_info->gr_gid);
  }
  chmod(socket_path_.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

  return true;
}

bool SocketProcessorImpl::ListenForClients() {
  if (SystemCallWrapper::GetWrapper().Listen(server_socket_,
                                             kMaxWaitingConnectReq) == -1) {
    LOG(ERROR) << __func__ << ": Unable to listen for clients.";
    CloseServer();
    return false;
  }
  return true;
}

void SocketProcessorImpl::PrintSocketErr(int ret_val, SocketDirection dir) {
  if (ret_val == -1) {
    LOG(WARNING) << __func__
                 << ": Unable to receive Thread frames from the Thread HAL, "
                 << "client socket: " << client_socket_
                 << ", direction: " << static_cast<int>(dir);
  } else if (ret_val == 0) {
    LOG(WARNING) << __func__
                 << ": Client connection is closed or send buffer is full, "
                 << "direction: " << static_cast<int>(dir);
  }
}

void SocketProcessor::Initialize(
    const std::string& socket_path,
    std::optional<HalPacketCallback> hal_packet_cb) {
  if (processor) {
    LOG(WARNING) << __func__ << "Already initialize the socket processor.";
    return;
  }
  CHECK(hal_packet_cb != std::nullopt)
      << __func__ << ": hal_packet_cb == nullptr";
  processor = new SocketProcessorImpl(socket_path, hal_packet_cb);
}

void SocketProcessor::Cleanup() {
  LOG(DEBUG) << __func__;
  if (!processor) {
    return;
  }

  SocketProcessorImpl* ptr = processor;
  processor = nullptr;

  delete ptr;
}

SocketProcessor* SocketProcessor::GetProcessor() {
  if (!processor) {
    LOG(FATAL) << __func__ << ": processor == nullptr.";
  }
  return processor;
}

}  // namespace thread
}  // namespace bluetooth_hal
