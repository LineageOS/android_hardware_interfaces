/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef WIFI_LEGACY_HAL_FACTORY_H_
#define WIFI_LEGACY_HAL_FACTORY_H_

#include <wifi_system/interface_tool.h>

#include "wifi_legacy_hal.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_5 {
namespace implementation {
// This is in a separate namespace to prevent typename conflicts between
// the legacy HAL types and the HIDL interface types.
namespace legacy_hal {
/**
 * Class that creates WifiLegacyHal objects for vendor HALs in the system.
 */
class WifiLegacyHalFactory {
   public:
    WifiLegacyHalFactory(
        const std::weak_ptr<wifi_system::InterfaceTool> iface_tool);
    virtual ~WifiLegacyHalFactory() = default;

    std::vector<std::shared_ptr<WifiLegacyHal>> getHals();

   private:
    typedef struct {
        wifi_hal_fn fn;
        bool primary;
        void* handle;
    } wifi_hal_lib_desc;

    bool initVendorHalDescriptorFromLinked();
    void initVendorHalsDescriptorList();
    bool initLinkedHalFunctionTable(wifi_hal_fn* hal_fn);
    bool loadVendorHalLib(const std::string& path, wifi_hal_lib_desc& desc);

    std::weak_ptr<wifi_system::InterfaceTool> iface_tool_;
    std::vector<wifi_hal_lib_desc> descs_;
    std::vector<std::shared_ptr<WifiLegacyHal>> legacy_hals_;
};

}  // namespace legacy_hal
}  // namespace implementation
}  // namespace V1_5
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_LEGACY_HAL_FACTORY_H_
