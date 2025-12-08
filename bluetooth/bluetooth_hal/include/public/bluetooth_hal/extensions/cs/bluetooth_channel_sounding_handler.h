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

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "aidl/android/hardware/bluetooth/ranging/BluetoothChannelSoundingParameters.h"
#include "aidl/android/hardware/bluetooth/ranging/CsSecurityLevel.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSession.h"
#include "aidl/android/hardware/bluetooth/ranging/IBluetoothChannelSoundingSessionCallback.h"
#include "aidl/android/hardware/bluetooth/ranging/SessionType.h"
#include "aidl/android/hardware/bluetooth/ranging/VendorSpecificData.h"
#include "bluetooth_hal/hal_packet.h"
#include "bluetooth_hal/hci_monitor.h"
#include "bluetooth_hal/hci_router_client.h"

namespace bluetooth_hal {
namespace extensions {
namespace cs {

class BluetoothChannelSoundingHandler
    : public ::bluetooth_hal::hci::HciRouterClient {
 public:
  struct SessionTracker {
    ::aidl::android::hardware::bluetooth::ranging::
        BluetoothChannelSoundingParameters parameters;
    uint16_t cur_procedure_counter{0xffff};
    bool is_fake_notification_enabled{false};
  };

  BluetoothChannelSoundingHandler();
  ~BluetoothChannelSoundingHandler();

  bool GetVendorSpecificData(
      std::optional<std::vector<std::optional<
          ::aidl::android::hardware::bluetooth::ranging::VendorSpecificData>>>*
          return_value);
  bool GetSupportedSessionTypes(
      std::optional<std::vector<
          ::aidl::android::hardware::bluetooth::ranging::SessionType>>*
          return_value);
  bool GetMaxSupportedCsSecurityLevel(
      ::aidl::android::hardware::bluetooth::ranging::CsSecurityLevel*
          return_value);
  bool OpenSession(
      const ::aidl::android::hardware::bluetooth::ranging::
          BluetoothChannelSoundingParameters& in_params,
      const std::shared_ptr<::aidl::android::hardware::bluetooth::ranging::
                                IBluetoothChannelSoundingSessionCallback>&
          in_callback,
      std::shared_ptr<::aidl::android::hardware::bluetooth::ranging::
                          IBluetoothChannelSoundingSession>* return_value);

 protected:
  void OnBluetoothChipReady() override {};
  void OnBluetoothChipClosed() override {};
  void OnBluetoothEnabled() override {};
  void OnBluetoothDisabled() override {};
  void OnCommandCallback(
      const ::bluetooth_hal::hci::HalPacket& packet) override;
  void OnMonitorPacketCallback(
      ::bluetooth_hal::hci::MonitorMode mode,
      const ::bluetooth_hal::hci::HalPacket& packet) override;

  std::optional<std::reference_wrapper<SessionTracker>> GetTracker(
      uint16_t connection_handle);

 private:
  void HandleCsSubevent(const ::bluetooth_hal::hci::HalPacket& packet);
  void HandleCsProcedureEnableCompleteEvent(
      const ::bluetooth_hal::hci::HalPacket& packet);

  ::bluetooth_hal::hci::HciBleMetaEventMonitor cs_data_subevent_monitor_;
  ::bluetooth_hal::hci::HciBleMetaEventMonitor
      cs_procedure_enable_subevent_monitor_;

  std::vector<uint8_t> local_capabilities_;

  std::unordered_map<uint16_t, SessionTracker> session_trackers_;
};

}  // namespace cs
}  // namespace extensions
}  // namespace bluetooth_hal
