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

#include "radio_modem_utils.h"

RadioModemIndication::RadioModemIndication(RadioServiceTest& parent) : parent_modem(parent) {}

ndk::ScopedAStatus RadioModemIndication::hardwareConfigChanged(
        RadioIndicationType /*type*/, const std::vector<HardwareConfig>& /*configs*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemIndication::modemReset(RadioIndicationType /*type*/,
                                                    const std::string& /*reason*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemIndication::radioCapabilityIndication(RadioIndicationType /*type*/,
                                                                   const RadioCapability& /*rc*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemIndication::radioStateChanged(RadioIndicationType /*type*/,
                                                           RadioState /*radioState*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemIndication::rilConnected(RadioIndicationType /*type*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioModemIndication::onImeiMappingChanged(RadioIndicationType /*type*/,
     const ::aidl::android::hardware::radio::modem::ImeiInfo& /*imeiInfo*/) {
    return ndk::ScopedAStatus::ok();
}
