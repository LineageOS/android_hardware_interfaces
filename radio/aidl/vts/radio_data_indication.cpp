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

#include "radio_data_utils.h"

RadioDataIndication::RadioDataIndication(RadioServiceTest& parent) : parent_data(parent) {}

ndk::ScopedAStatus RadioDataIndication::dataCallListChanged(
        RadioIndicationType /*type*/, const std::vector<SetupDataCallResult>& /*dcList*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataIndication::keepaliveStatus(RadioIndicationType /*type*/,
                                                        const KeepaliveStatus& /*status*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataIndication::pcoData(RadioIndicationType /*type*/,
                                                const PcoDataInfo& /*pco*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataIndication::unthrottleApn(RadioIndicationType /*type*/,
                                                      const DataProfileInfo& /*dataProfileInfo*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioDataIndication::slicingConfigChanged(
        RadioIndicationType /*type*/, const SlicingConfig& /*slicingConfig*/) {
    return ndk::ScopedAStatus::ok();
}
