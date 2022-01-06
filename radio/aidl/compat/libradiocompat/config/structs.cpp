/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "structs.h"

#include "collections.h"

#include <android-base/logging.h>

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::config;

uint32_t toHidl(const aidl::SlotPortMapping& slotPortMapping) {
    if (slotPortMapping.portId != 0) {
        LOG(ERROR) << "Port ID " << slotPortMapping.portId << " != 0 not supported by HIDL HAL";
    }
    return slotPortMapping.physicalSlotId;
}

aidl::SimSlotStatus toAidl(const config::V1_0::SimSlotStatus& sst) {
    return toAidl({sst, ""});
}

aidl::SimSlotStatus toAidl(const config::V1_2::SimSlotStatus& sst) {
    const aidl::SimPortInfo portInfo = {
            .iccId = sst.base.iccid,
            .logicalSlotId = static_cast<int32_t>(sst.base.logicalSlotId),
            .portActive = sst.base.slotState == config::V1_0::SlotState::ACTIVE,
    };

    return {
            .cardState = static_cast<int32_t>(sst.base.cardState),
            .atr = sst.base.atr,
            .eid = sst.eid,
            .portInfo = {portInfo},
    };
}

uint8_t toAidl(const config::V1_1::ModemInfo& info) {
    return info.modemId;
}

aidl::PhoneCapability toAidl(const config::V1_1::PhoneCapability& phoneCapability) {
    return {
            .maxActiveData = static_cast<int8_t>(phoneCapability.maxActiveData),
            .maxActiveInternetData = static_cast<int8_t>(phoneCapability.maxActiveInternetData),
            .isInternetLingeringSupported = phoneCapability.isInternetLingeringSupported,
            .logicalModemIds = toAidl(phoneCapability.logicalModemList),
    };
}

}  // namespace android::hardware::radio::compat
