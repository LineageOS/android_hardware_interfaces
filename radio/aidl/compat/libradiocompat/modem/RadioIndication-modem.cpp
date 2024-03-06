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

#include <libradiocompat/RadioIndication.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"

#include "collections.h"

#define RADIO_MODULE "ModemIndication"

namespace android::hardware::radio::compat {

namespace aidl = ::aidl::android::hardware::radio::modem;

void RadioIndication::setResponseFunction(std::shared_ptr<aidl::IRadioModemIndication> modemCb) {
    mModemCb = modemCb;
}

std::shared_ptr<aidl::IRadioModemIndication> RadioIndication::modemCb() {
    return mModemCb.get();
}

Return<void> RadioIndication::hardwareConfigChanged(V1_0::RadioIndicationType type,
                                                    const hidl_vec<V1_0::HardwareConfig>& configs) {
    LOG_CALL << type;
    modemCb()->hardwareConfigChanged(toAidl(type), toAidl(configs));
    return {};
}

Return<void> RadioIndication::modemReset(V1_0::RadioIndicationType type, const hidl_string& reasn) {
    LOG_CALL << type;
    modemCb()->modemReset(toAidl(type), reasn);
    return {};
}

Return<void> RadioIndication::radioCapabilityIndication(V1_0::RadioIndicationType type,
                                                        const V1_0::RadioCapability& rc) {
    LOG_CALL << type;
    modemCb()->radioCapabilityIndication(toAidl(type), toAidl(rc));
    return {};
}

Return<void> RadioIndication::radioStateChanged(V1_0::RadioIndicationType t, V1_0::RadioState st) {
    LOG_CALL << t;
    modemCb()->radioStateChanged(toAidl(t), aidl::RadioState(st));
    return {};
}

Return<void> RadioIndication::rilConnected(V1_0::RadioIndicationType type) {
    LOG_CALL << type;
    modemCb()->rilConnected(toAidl(type));
    return {};
}

Return<void> RadioIndication::onImeiMappingChanged(V1_0::RadioIndicationType type,
                                    ::aidl::android::hardware::radio::modem::ImeiInfo imeiInfo) {
    LOG_CALL << type;
    modemCb()->onImeiMappingChanged(toAidl(type), imeiInfo);
    return {};
}

}  // namespace android::hardware::radio::compat
