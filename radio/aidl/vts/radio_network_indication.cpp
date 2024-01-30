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

#include "radio_network_utils.h"

RadioNetworkIndication::RadioNetworkIndication(RadioServiceTest& parent) : parent_network(parent) {}

ndk::ScopedAStatus RadioNetworkIndication::barringInfoChanged(
        RadioIndicationType /*type*/, const CellIdentity& /*cellIdentity*/,
        const std::vector<BarringInfo>& /*barringInfos*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::cdmaPrlChanged(RadioIndicationType /*type*/,
                                                          int32_t /*version*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::cellInfoList(RadioIndicationType /*type*/,
                                                        const std::vector<CellInfo>& /*records*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::currentLinkCapacityEstimate(
        RadioIndicationType /*type*/, const LinkCapacityEstimate& /*lce*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::currentPhysicalChannelConfigs(
        RadioIndicationType /*type*/, const std::vector<PhysicalChannelConfig>& /*configs*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::currentSignalStrength(
        RadioIndicationType /*type*/, const SignalStrength& /*signalStrength*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::imsNetworkStateChanged(RadioIndicationType /*type*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::networkScanResult(RadioIndicationType /*type*/,
                                                             const NetworkScanResult& /*result*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::networkStateChanged(RadioIndicationType /*type*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::nitzTimeReceived(RadioIndicationType /*type*/,
                                                            const std::string& /*nitzTime*/,
                                                            int64_t /*receivedTime*/,
                                                            int64_t /*age*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::registrationFailed(RadioIndicationType /*type*/,
                                                              const CellIdentity& /*cellIdentity*/,
                                                              const std::string& /*chosenPlmn*/,
                                                              int32_t /*domain*/,
                                                              int32_t /*causeCode*/,
                                                              int32_t /*additionalCauseCode*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::restrictedStateChanged(RadioIndicationType /*type*/,
                                                                  PhoneRestrictedState /*state*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::suppSvcNotify(RadioIndicationType /*type*/,
                                                         const SuppSvcNotification& /*suppSvc*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::voiceRadioTechChanged(RadioIndicationType /*type*/,
                                                                 RadioTechnology /*rat*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::emergencyNetworkScanResult(
        RadioIndicationType /*type*/, const EmergencyRegResult& /*result*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::cellularIdentifierDisclosed(
        RadioIndicationType /*type*/,
        const CellularIdentifierDisclosure& /*disclosures*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus RadioNetworkIndication::securityAlgorithmsUpdated(
        RadioIndicationType /*type*/, const SecurityAlgorithmUpdate& /*securityAlgorithmUpdate*/) {
    return ndk::ScopedAStatus::ok();
}
