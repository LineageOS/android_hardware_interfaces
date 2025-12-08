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

#define LOG_TAG "bluetooth_hal.transport.uart_h4"

#include "bluetooth_hal/transport/uart_h4/transport_uart_h4.h"

#include <memory>
#include <mutex>

#include "android-base/logging.h"
#include "bluetooth_hal/config/hal_config_loader.h"
#include "bluetooth_hal/debug/debug_central.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hal_types.h"
#include "bluetooth_hal/transport/device_control/power_manager.h"
#include "bluetooth_hal/transport/device_control/uart_manager.h"
#include "bluetooth_hal/transport/transport_interface.h"
#include "bluetooth_hal/transport/uart_h4/data_processor.h"
#include "bluetooth_hal/transport/vendor_packet_validator_interface.h"
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
  LOG(INFO) << __func__ << ": Initializing UART H4 transport.";
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
        LOG(VERBOSE)
            << __func__
            << ": Packet ready from data processor, notifying callback.";
        transport_interface_callback_->OnTransportPacketReady(packet);
      });
  data_processor_->StartProcessing();

  LOG(INFO) << __func__ << ": Initialization is completed.";

  return true;
}

void TransportUartH4::Cleanup() {
  LOG(INFO) << __func__ << ": Cleaning up UART H4 transport.";
  TransportInterface::Unsubscribe(*this);
  data_processor_.reset();
  TerminateDataPath();
  TeardownLowPowerMode();
  PowerManager::PowerControl(false);
  if (transport_interface_callback_) {
    transport_interface_callback_->OnTransportClosed();
  }
}

bool TransportUartH4::IsTransportActive() const {
  bool active = uart_fd_.ok();
  LOG(VERBOSE) << __func__ << ": UART FD is " << (active ? "valid" : "invalid")
               << ", transport is " << (active ? "active" : "inactive");
  return active;
}

bool TransportUartH4::Send(const HalPacket& packet) {
  if (!data_processor_) {
    return false;
  }

  if (!ResumeFromLowPowerMode()) {
    LOG(WARNING)
        << __func__
        << ": Failed to resume from low power mode after sending packet.";
  }

  bool sent_successfully =
      data_processor_->Send(std::span(packet)) == packet.size();
  if (!sent_successfully) {
    LOG(ERROR) << __func__ << ": Failed to send packet.";
  }

  RefreshLpmTimer();

  return sent_successfully;
}

void TransportUartH4::RefreshLpmTimer() {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  if (is_lpm_resumed_) {
    low_power_timer_.Schedule(
        std::bind_front(&TransportUartH4::SuspendToLowPowerMode, this),
        std::chrono::milliseconds{kLpmTimeoutMs});
  }
}

bool TransportUartH4::ResumeFromLowPowerMode() {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  HAL_LOG(DEBUG) << __func__ << ": Attempting to resume from low power mode.";

  if (!HalConfigLoader::GetLoader().IsLowPowerModeSupported() ||
      !IsLowPowerModeSetupCompleted() || is_lpm_resumed_) {
    LOG(VERBOSE) << __func__ << ": LPM not supported ("
                 << HalConfigLoader::GetLoader().IsLowPowerModeSupported()
                 << "), or not setup (" << IsLowPowerModeSetupCompleted()
                 << "), or already resumed (" << is_lpm_resumed_
                 << "). Skipping resume.";
    return true;
  }

  if (IsTransportWakelockEnabled()) {
    Wakelock::GetWakelock().Acquire(WakeSource::kTransport);
  }

  if (!PowerManager::ResumeFromLowPowerMode()) {
    LOG(ERROR) << __func__
               << ": PowerManager failed to resume from low power mode.";
    return false;
  }

  is_lpm_resumed_ = true;
  HAL_LOG(DEBUG) << __func__ << ": Successfully resumed from low power mode.";

  return true;
}

bool TransportUartH4::SuspendToLowPowerMode() {
  std::unique_lock<std::recursive_mutex> lock(mutex_);
  HAL_LOG(DEBUG) << __func__ << ": Attempting to suspend to low power mode.";

  if (!HalConfigLoader::GetLoader().IsLowPowerModeSupported() ||
      !IsLowPowerModeSetupCompleted() || !is_lpm_resumed_) {
    LOG(VERBOSE)
        << __func__
        << ": LPM not supported, or not setup, or not resumed. Skipping "
           "suspend.";
    return true;
  }

  if (IsTransportWakelockEnabled()) {
    Wakelock::GetWakelock().Release(WakeSource::kTransport);
  }

  if (!PowerManager::SuspendToLowPowerMode()) {
    LOG(ERROR) << __func__
               << ": PowerManager failed to suspend to low power mode.";
    return false;
  }

  is_lpm_resumed_ = false;
  HAL_LOG(DEBUG) << __func__ << ": Successfully suspend to low power mode.";

  return true;
}

bool TransportUartH4::IsLowPowerModeSetupCompleted() const {
  bool completed = PowerManager::IsLowPowerModeSetupCompleted();
  LOG(VERBOSE) << __func__ << ": Low power mode setup is "
               << (completed ? "completed" : "not completed") << ".";
  return completed;
}

bool TransportUartH4::InitializeDataPath() {
  bool success = UartManager::Open();
  LOG(INFO) << __func__
            << ": UART open: " << (success ? "successfully." : "failed.");
  return success;
};

void TransportUartH4::TerminateDataPath() {
  LOG(DEBUG) << __func__ << ": Terminating data path (UART close).";
  UartManager::Close();
};

bool TransportUartH4::SetupLowPowerMode() {
  if (!HalConfigLoader::GetLoader().IsLowPowerModeSupported()) {
    LOG(INFO) << __func__
              << ": Low power mode not supported by config. Skipping setup.";
    return true;
  }

  bool success = PowerManager::SetupLowPowerMode();
  LOG(INFO) << __func__ << ": Low power mode setup "
            << (success ? "succeeded" : "failed") << ".";

  return success;
};

void TransportUartH4::TeardownLowPowerMode() {
  LOG(DEBUG) << __func__ << ": Tearing down low power mode.";

  if (!HalConfigLoader::GetLoader().IsLowPowerModeSupported()) {
    LOG(INFO) << __func__
              << ": Low power mode not supported by config. Skipping teardown.";
    return;
  }

  low_power_timer_.Cancel();
  SuspendToLowPowerMode();
  PowerManager::TeardownLowPowerMode();
};

void TransportUartH4::NotifyHalStateChange(HalState hal_state) {
  LOG(INFO) << __func__ << ": HAL state changed to "
            << HalStateToString(hal_state) << " ("
            << static_cast<int>(hal_state) << ")";
  switch (hal_state) {
    case HalState::kPreFirmwareDownload:
    case HalState::kFirmwareDownloadCompleted: {
      const auto baud_rate = BaudRate::kRate115200;
      LOG(DEBUG) << __func__ << ": Updating UART baud rate to "
                 << static_cast<int>(baud_rate) << " for state "
                 << static_cast<int>(hal_state);
      UartManager::UpdateBaudRate(baud_rate);
      break;
    }
    case HalState::kFirmwareDownloading:
    case HalState::kFirmwareReady: {
      const auto baud_rate =
          HalConfigLoader::GetLoader().GetUartBaudRate(TransportType::kUartH4);
      LOG(DEBUG) << __func__ << ": Updating UART baud rate to "
                 << static_cast<int>(baud_rate) << " for state "
                 << static_cast<int>(hal_state);
      UartManager::UpdateBaudRate(baud_rate);
      if (hal_state == HalState::kFirmwareReady) {
        LOG(DEBUG) << __func__ << ": Setting up LPM for FirmwareReady state.";
        SetupLowPowerMode();
        ResumeFromLowPowerMode();
      }
      break;
    }
    default:
      LOG(DEBUG) << __func__ << ": No action for HAL state "
                 << static_cast<int>(hal_state);
      break;
  }
}

void TransportUartH4::EnableTransportWakelock(bool enable) {
  LOG(INFO) << __func__ << ": Transport wakelock "
            << (enable ? "enabled" : "disabled") << ".";
  transport_wakelock_enabled_ = enable;
}

bool TransportUartH4::IsTransportWakelockEnabled() {
  LOG(VERBOSE) << __func__ << ": Transport wakelock is "
               << (transport_wakelock_enabled_ ? "enabled" : "disabled") << ".";
  return transport_wakelock_enabled_;
}

void TransportUartH4::RegisterVendorPacketValidator(
    VendorPacketValidatorInterface::FactoryFn factory) {
  VendorPacketValidatorInterface::RegisterVendorPacketValidator(
      std::move(factory));
}

}  // namespace transport
}  // namespace bluetooth_hal
