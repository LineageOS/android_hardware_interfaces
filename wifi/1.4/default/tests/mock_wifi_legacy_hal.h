/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef MOCK_WIFI_LEGACY_HAL_H_
#define MOCK_WIFI_LEGACY_HAL_H_

#include <gmock/gmock.h>

#include "wifi_legacy_hal.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_4 {
namespace implementation {
namespace legacy_hal {

class MockWifiLegacyHal : public WifiLegacyHal {
   public:
    MockWifiLegacyHal(
        const std::weak_ptr<wifi_system::InterfaceTool> iface_tool);
    MOCK_METHOD0(initialize, wifi_error());
    MOCK_METHOD0(start, wifi_error());
    MOCK_METHOD2(stop, wifi_error(std::unique_lock<std::recursive_mutex>*,
                                  const std::function<void()>&));
    MOCK_METHOD2(setDfsFlag, wifi_error(const std::string&, bool));
    MOCK_METHOD2(registerRadioModeChangeCallbackHandler,
                 wifi_error(const std::string&,
                            const on_radio_mode_change_callback&));
    MOCK_METHOD1(getFirmwareVersion, std::pair<wifi_error, std::string>(
                                         const std::string& iface_name));
    MOCK_METHOD1(getDriverVersion, std::pair<wifi_error, std::string>(
                                       const std::string& iface_name));

    MOCK_METHOD2(selectTxPowerScenario,
                 wifi_error(const std::string& iface_name,
                            wifi_power_scenario scenario));
    MOCK_METHOD1(resetTxPowerScenario,
                 wifi_error(const std::string& iface_name));
    MOCK_METHOD2(nanRegisterCallbackHandlers,
                 wifi_error(const std::string&, const NanCallbackHandlers&));
    MOCK_METHOD2(nanDisableRequest,
                 wifi_error(const std::string&, transaction_id));
    MOCK_METHOD3(nanDataInterfaceDelete,
                 wifi_error(const std::string&, transaction_id,
                            const std::string&));
    MOCK_METHOD2(createVirtualInterface,
                 wifi_error(const std::string& ifname,
                            wifi_interface_type iftype));
    MOCK_METHOD1(deleteVirtualInterface, wifi_error(const std::string& ifname));
};
}  // namespace legacy_hal
}  // namespace implementation
}  // namespace V1_4
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // MOCK_WIFI_LEGACY_HAL_H_
