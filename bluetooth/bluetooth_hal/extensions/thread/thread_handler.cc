/*
 * Copyright 2025 The Android Open Source Project
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

#include "bluetooth_hal/extensions/thread/thread_handler.h"

#include <memory>
#include <mutex>

#include "android-base/logging.h"
#include "bluetooth_hal/extensions/thread/thread_daemon.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hci_monitor.h"

namespace bluetooth_hal {
namespace thread {

using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::hci::MonitorMode;
using ::bluetooth_hal::thread::ThreadDaemon;

ThreadHandler::ThreadHandler() {
  thread_daemon_ = std::make_unique<ThreadDaemon>(
      [this](const ::bluetooth_hal::hci::HalPacket& packet) {
        this->SendData(packet);
      });
  RegisterMonitor(thread_data_monitor_, MonitorMode::kIntercept);
}

ThreadHandler::~ThreadHandler() {
  UnregisterMonitor(thread_data_monitor_);
  thread_daemon_.reset();
}

void ThreadHandler::Initialize() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!handler_) {
    handler_ = std::make_unique<ThreadHandler>();
  }
}

void ThreadHandler::Cleanup() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!handler_) {
    return;
  }
  handler_.reset();
}

bool ThreadHandler::IsHandlerRunning() {
  std::lock_guard<std::mutex> lock(mutex_);
  return handler_ != nullptr;
}

ThreadHandler& ThreadHandler::GetHandler() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!handler_) {
    LOG(FATAL) << __func__ << ": handler == nullptr.";
  }
  return *handler_;
}

void ThreadHandler::OnCommandCallback(
    [[maybe_unused]] const HalPacket& packet) {}

void ThreadHandler::OnMonitorPacketCallback([[maybe_unused]] MonitorMode mode,
                                            const HalPacket& packet) {
  std::lock_guard<std::mutex> guard(mutex_);
  if (packet.GetType() == HciPacketType::kThreadData && thread_daemon_) {
    thread_daemon_->SendUplink(packet);
  }
}

void ThreadHandler::OnBluetoothChipReady() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (thread_daemon_) {
    thread_daemon_->Start();
  }
}

void ThreadHandler::OnBluetoothChipClosed() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (thread_daemon_) {
    thread_daemon_->Stop();
  }
}

void ThreadHandler::OnBluetoothEnabled() {}

void ThreadHandler::OnBluetoothDisabled() {}

bool ThreadHandler::IsDaemonRunning() const {
  if (thread_daemon_) {
    return thread_daemon_->IsDaemonRunning();
  }
  return false;
}

}  // namespace thread
}  // namespace bluetooth_hal
