/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef DRM_HAL_CLEARKEY_MODULE_H
#define DRM_HAL_CLEARKEY_MODULE_H

#include "drm_hal_vendor_module_api.h"

namespace android {
namespace hardware {
namespace drm {
namespace V1_2 {
namespace vts {

class DrmHalVTSClearkeyModule : public DrmHalVTSVendorModule_V1 {
   public:
    DrmHalVTSClearkeyModule() {}
    virtual ~DrmHalVTSClearkeyModule() {}

    virtual uint32_t getAPIVersion() const override { return 1; }

    virtual std::string getServiceName() const override { return "clearkey"; }

    virtual std::vector<uint8_t> getUUID() const override {
        return {0xE2, 0x71, 0x9D, 0x58, 0xA9, 0x85, 0xB3, 0xC9,
                0x78, 0x1A, 0xB0, 0x30, 0xAF, 0x78, 0xD3, 0x0E };
    }


    virtual std::vector<uint8_t> handleProvisioningRequest(
            const std::vector<uint8_t>& provisioningRequest,
            const std::string& url) override;

    virtual std::vector<DrmHalVTSClearkeyModule::ContentConfiguration>
            getContentConfigurations() const override;

    virtual std::vector<uint8_t> handleKeyRequest(
            const std::vector<uint8_t>& keyRequest,
            const std::string& serverUrl) override;
};

}  // namespace vts
}  // namespace V1_2
}  // namespace drm
}  // namespace hardware
}  // namespace android

#endif  // DRM_HAL_CLEARKEY_MODULE_H
