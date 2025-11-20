/*
 * Copyright 2024 The Android Open Source Project
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

#define LOG_TAG "bthal.transport.uart_h4"

#include "bluetooth_hal/transport/uart_h4/transport_uart_h4.h"

#include <memory>
#include <mutex>

#include "android-base/logging.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/device_control/power_manager.h"
#include "bluetooth_hal/transport/device_control/uart_manager.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "bluetooth_hal/transport/uart_h4/data_processor.h"
#include "bluetooth_hal/util/android_base_wrapper.h"
#include "bluetooth_hal/util/power/wakelock.h"
#include "bluetooth_hal/util/timer_manager.h"

namespace bluetooth_hal {
namespace transport {

using ::bluetooth_hal::HalState;
using ::bluetooth_hal::Property;
using ::bluetooth_hal::config::HalConfigLoader;
using ::bluetooth_hal::hci::HalPacket;
using ::bluetooth_hal::hci::HalPacketCallback;
using ::bluetooth_hal::hci::HciPacketType;
using ::bluetooth_hal::uart::BaudRate;
using ::bluetooth_hal::util::AndroidBaseWrapper;
using ::bluetooth_hal::util::Timer;
using ::bluetooth_hal::util::power::Wakelock;
using ::bluetooth_hal::util::power::WakeSource;

TransportUartH4::~TransportUartH4() {
  if (transport_interface_callback_) {
    transport_interface_callback_->OnTransportClosed();
  }
}

TransportType TransportUartH4::GetInstanceTransportType() const {
  return TransportType::kUartH4;
}

bool TransportUartH4::Initialize(
    TransportInterfaceCallback* transport_interface_callback) {
  TransportInterface::Subscribe(*this);

  transport_interface_callback_ = transport_interface_callback;

  // Power on the underlying device.
  PowerManager::PowerControl(false);
  if (!PowerManager::PowerControl(true)) {
    LOG(ERROR) << __func__ << ": Cannot power on the device.";
    Cleanup();
    return false;
  }

  // Initialize data channel to the device.
  if (!InitializeDataPath()) {
    LOG(ERROR) << __func__ << ": Cannot initialize the data path.";
    Cleanup();
    return false;
  }

  PowerManager::ConfigRxWakelockTime(
      HalConfigLoader::GetLoader().GetKernelRxWakelockTimeMilliseconds());

  if (!IsTransportActive()) {
    LOG(ERROR) << __func__ << ": Transport is not active.";
    Cleanup();
    return false;
  }

  AndroidBaseWrapper::GetWrapper().SetProperty(Property::kLastUartPath, "apc");

  data_processor_ = std::make_unique<DataProcessor>(
      uart_fd_.get(), [&](const HalPacket& packet) {
        transport_interface_callback_->OnTransportPacketReady(packet);
      });
  data_processor_->StartProcessing();

  LOG(INFO) << __func__ << ": Initialization is completed.";
  return true;
}

void TransportUartH4::Cleanup() {
  TransportInterface::Unsubscribe(*this);
  data_processor_.reset();
  TerminateDataPath();
  TeardownLowPowerMode();
  PowerManager::PowerControl(false);
  if (transport_interface_callback_) {
    transport_interface_callback_->OnTransportClosed();
  }
}

bool TransportUartH4::IsTransportActive() const { return uart_fd_.ok(); }

bool TransportUartH4::Send(const HalPacket& packet) {
  // TODO: b/401131063 - Handle LPM here once the timer util is ready.
  if (!data_processor_) {
    return false;
  }
  ResumeFromLowPowerMode();
  return data_processor_->Send(std::span(packet)) == packet.size();
}

bool TransportUartH4::ResumeFromLowPowerMode() {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (!HalConfigLoader::GetLoader().IsLowPowerModeSupported() ||
      !IsLowPowerModeSetupCompleted() || is_lpm_resumed_) {
    return true;
  }
  if (IsTransportWakelockEnabled()) {
    Wakelock::GetWakelock().Acquire(WakeSource::kTransport);
  }
  low_power_timer_.Schedule(
      std::bind_front(&TransportUartH4::SuspendToLowPowerMode, this),
      std::chrono::milliseconds{kLpmTimeoutMs});
  if (!PowerManager::ResumeFromLowPowerMode()) {
    return false;
  }
  is_lpm_resumed_ = true;
  return true;
}

bool TransportUartH4::SuspendToLowPowerMode() {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (!HalConfigLoader::GetLoader().IsLowPowerModeSupported() ||
      !IsLowPowerModeSetupCompleted() || !is_lpm_resumed_) {
    return true;
  }
  if (IsTransportWakelockEnabled()) {
    Wakelock::GetWakelock().Release(WakeSource::kTransport);
  }
  if (!PowerManager::SuspendToLowPowerMode()) {
    return false;
  }
  is_lpm_resumed_ = false;
  return true;
}

bool TransportUartH4::IsLowPowerModeSetupCompleted() const {
  return PowerManager::IsLowPowerModeSetupCompleted();
}

bool TransportUartH4::InitializeDataPath() { return UartManager::Open(); };

void TransportUartH4::TerminateDataPath() { UartManager::Close(); };

bool TransportUartH4::SetupLowPowerMode() {
  if (!HalConfigLoader::GetLoader().IsLowPowerModeSupported()) {
    return true;
  }
  return PowerManager::SetupLowPowerMode();
};

void TransportUartH4::TeardownLowPowerMode() {
  if (!HalConfigLoader::GetLoader().IsLowPowerModeSupported()) {
    return;
  }
  low_power_timer_.Cancel();
  SuspendToLowPowerMode();
  PowerManager::TeardownLowPowerMode();
};

void TransportUartH4::NotifyHalStateChange(HalState hal_state) {
  switch (hal_state) {
    case HalState::kFirmwareDownloading:
      UartManager::UpdateBaudRate(
          HalConfigLoader::GetLoader().GetUartBaudRate(TransportType::kUartH4));
      break;
    case HalState::kFirmwareDownloadCompleted:
      UartManager::UpdateBaudRate(BaudRate::kRate115200);
      break;
    case HalState::kFirmwareReady:
      UartManager::UpdateBaudRate(
          HalConfigLoader::GetLoader().GetUartBaudRate(TransportType::kUartH4));
      SetupLowPowerMode();
      ResumeFromLowPowerMode();
      break;
    default:
      break;
  }
}

void TransportUartH4::EnableTransportWakelock(bool enable) {
  transport_wakelock_enabled_ = enable;
}

bool TransportUartH4::IsTransportWakelockEnabled() {
  return transport_wakelock_enabled_;
}

}  // namespace transport
}  // namespace bluetooth_hal
