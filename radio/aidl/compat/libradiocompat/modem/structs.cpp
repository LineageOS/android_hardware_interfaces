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

#include "commonStructs.h"

#include "collections.h"

#include <android-base/logging.h>

namespace android::hardware::radio::compat {

using ::aidl::android::hardware::radio::AccessNetwork;
using ::aidl::android::hardware::radio::RadioTechnology;
namespace aidl = ::aidl::android::hardware::radio::modem;

V1_0::NvWriteItem toHidl(const aidl::NvWriteItem& item) {
    return {
            .itemId = static_cast<V1_0::NvItem>(item.itemId),
            .value = item.value,
    };
}

aidl::RadioCapability toAidl(const V1_0::RadioCapability& capa) {
    return {
            .session = capa.session,
            .phase = static_cast<int32_t>(capa.phase),
            .raf = static_cast<int32_t>(capa.raf),
            .logicalModemUuid = capa.logicalModemUuid,
            .status = static_cast<int32_t>(capa.status),
    };
}

V1_0::RadioCapability toHidl(const aidl::RadioCapability& capa) {
    return {
            .session = capa.session,
            .phase = static_cast<V1_0::RadioCapabilityPhase>(capa.phase),
            .raf = toHidlBitfield<V1_0::RadioAccessFamily>(capa.raf),
            .logicalModemUuid = capa.logicalModemUuid,
            .status = static_cast<V1_0::RadioCapabilityStatus>(capa.status),
    };
}

aidl::HardwareConfig toAidl(const V1_0::HardwareConfig& config) {
    return {
            .type = static_cast<int32_t>(config.type),
            .uuid = config.uuid,
            .state = static_cast<int32_t>(config.state),
            .modem = toAidl(config.modem),
            .sim = toAidl(config.sim),
    };
}

aidl::HardwareConfigModem toAidl(const V1_0::HardwareConfigModem& modem) {
    return {
            .rilModel = modem.rilModel,
            .rat = RadioTechnology(modem.rat),
            .maxVoiceCalls = modem.maxVoice,
            .maxDataCalls = modem.maxData,
            .maxStandby = modem.maxStandby,
    };
}

aidl::HardwareConfigSim toAidl(const V1_0::HardwareConfigSim& sim) {
    return {
            .modemUuid = sim.modemUuid,
    };
}

aidl::ActivityStatsInfo toAidl(const V1_0::ActivityStatsInfo& info) {
    const aidl::ActivityStatsTechSpecificInfo techSpecificInfo = {
            .rat = AccessNetwork(AccessNetwork::UNKNOWN),
            .frequencyRange = static_cast<int32_t>(
                    aidl::ActivityStatsTechSpecificInfo::FREQUENCY_RANGE_UNKNOWN),
            .txmModetimeMs = toAidl(info.txmModetimeMs),
            .rxModeTimeMs = static_cast<int32_t>(info.rxModeTimeMs),
    };

    return {
            .sleepModeTimeMs = static_cast<int32_t>(info.sleepModeTimeMs),
            .idleModeTimeMs = static_cast<int32_t>(info.idleModeTimeMs),
            .techSpecificInfo = {techSpecificInfo},
    };
}

}  // namespace android::hardware::radio::compat
