/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "radio_satellite_utils.h"

RadioSatelliteIndication::RadioSatelliteIndication(RadioServiceTest& parent)
    : parent_satellite(parent) {}

ndk::ScopedAStatus RadioSatelliteIndication::onPendingMessageCount(RadioIndicationType /*type*/,
                                                                   int32_t /*count*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteIndication::onNewMessages(
        RadioIndicationType /*type*/, const std::vector<std::string>& /*messages*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteIndication::onMessagesTransferComplete(
        RadioIndicationType /*type*/, bool /*complete*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteIndication::onSatellitePointingInfoChanged(
        RadioIndicationType /*type*/, const PointingInfo& /*pointingInfo*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteIndication::onSatelliteModeChanged(RadioIndicationType /*type*/,
                                                                    SatelliteMode /*mode*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteIndication::onSatelliteRadioTechnologyChanged(
        RadioIndicationType /*type*/, NTRadioTechnology /*technology*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioSatelliteIndication::onProvisionStateChanged(
        RadioIndicationType /*type*/, bool /*provisioned*/,
        const std::vector<SatelliteFeature>& /*features*/) {
    return ndk::ScopedAStatus::ok();
}
