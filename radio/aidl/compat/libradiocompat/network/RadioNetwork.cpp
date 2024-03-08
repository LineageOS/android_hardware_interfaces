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

#include <libradiocompat/RadioNetwork.h>

#include "commonStructs.h"
#include "debug.h"
#include "structs.h"
#include "utils.h"

#include "collections.h"

#define RADIO_MODULE "Network"

namespace android::hardware::radio::compat {

using ::aidl::android::hardware::radio::AccessNetwork;
using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::network;
constexpr auto ok = &ScopedAStatus::ok;

std::shared_ptr<aidl::IRadioNetworkResponse> RadioNetwork::respond() {
    return mCallbackManager->response().networkCb();
}

ScopedAStatus RadioNetwork::getAllowedNetworkTypesBitmap(int32_t serial) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->getAllowedNetworkTypesBitmap(serial);
    } else {
        mHal1_5->getPreferredNetworkType(serial);
    }
    return ok();
}

ScopedAStatus RadioNetwork::getAvailableBandModes(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getAvailableBandModes(serial);
    return ok();
}

ScopedAStatus RadioNetwork::getAvailableNetworks(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getAvailableNetworks(serial);
    return ok();
}

ScopedAStatus RadioNetwork::getBarringInfo(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getBarringInfo(serial);
    return ok();
}

ScopedAStatus RadioNetwork::getCdmaRoamingPreference(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getCdmaRoamingPreference(serial);
    return ok();
}

ScopedAStatus RadioNetwork::getCellInfoList(int32_t serial) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->getCellInfoList_1_6(serial);
    } else {
        mHal1_5->getCellInfoList(serial);
    }
    return ok();
}

ScopedAStatus RadioNetwork::getDataRegistrationState(int32_t serial) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->getDataRegistrationState_1_6(serial);
    } else {
        mHal1_5->getDataRegistrationState_1_5(serial);
    }
    return ok();
}

ScopedAStatus RadioNetwork::getImsRegistrationState(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getImsRegistrationState(serial);
    return ok();
}

ScopedAStatus RadioNetwork::getNetworkSelectionMode(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getNetworkSelectionMode(serial);
    return ok();
}

ScopedAStatus RadioNetwork::getOperator(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getOperator(serial);
    return ok();
}

ScopedAStatus RadioNetwork::getSignalStrength(int32_t serial) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->getSignalStrength_1_6(serial);
    } else {
        mHal1_5->getSignalStrength_1_4(serial);
    }
    return ok();
}

ScopedAStatus RadioNetwork::getSystemSelectionChannels(int32_t serial) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->getSystemSelectionChannels(serial);
    } else {
        respond()->getSystemSelectionChannelsResponse(notSupported(serial), {});
    }
    return ok();
}

ScopedAStatus RadioNetwork::getVoiceRadioTechnology(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->getVoiceRadioTechnology(serial);
    return ok();
}

ScopedAStatus RadioNetwork::getVoiceRegistrationState(int32_t serial) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->getVoiceRegistrationState_1_6(serial);
    } else {
        mHal1_5->getVoiceRegistrationState_1_5(serial);
    }
    return ok();
}

ScopedAStatus RadioNetwork::isNrDualConnectivityEnabled(int32_t serial) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->isNrDualConnectivityEnabled(serial);
    } else {
        respond()->isNrDualConnectivityEnabledResponse(notSupported(serial), false);
    }
    return ok();
}

ScopedAStatus RadioNetwork::responseAcknowledgement() {
    LOG_CALL;
    mHal1_5->responseAcknowledgement();
    return ok();
}

ScopedAStatus RadioNetwork::setAllowedNetworkTypesBitmap(int32_t serial, int32_t ntype) {
    LOG_CALL << serial;
    const auto raf = toHidlBitfield<V1_4::RadioAccessFamily>(ntype);
    if (mHal1_6) {
        mHal1_6->setAllowedNetworkTypesBitmap(serial, raf);
    } else {
        mHal1_5->setPreferredNetworkType(serial, getNetworkTypeFromRaf(raf));
    }
    return ok();
}

ScopedAStatus RadioNetwork::setBandMode(int32_t serial, aidl::RadioBandMode mode) {
    LOG_CALL << serial;
    mHal1_5->setBandMode(serial, V1_0::RadioBandMode(mode));
    return ok();
}

ScopedAStatus RadioNetwork::setBarringPassword(int32_t serial, const std::string& facility,
                                               const std::string& oldPw, const std::string& newPw) {
    LOG_CALL << serial;
    mHal1_5->setBarringPassword(serial, facility, oldPw, newPw);
    return ok();
}

ScopedAStatus RadioNetwork::setCdmaRoamingPreference(int32_t serial, aidl::CdmaRoamingType type) {
    LOG_CALL << serial;
    mHal1_5->setCdmaRoamingPreference(serial, V1_0::CdmaRoamingType(type));
    return ok();
}

ScopedAStatus RadioNetwork::setCellInfoListRate(int32_t serial, int32_t rate) {
    LOG_CALL << serial;
    mHal1_5->setCellInfoListRate(serial, rate);
    return ok();
}

ScopedAStatus RadioNetwork::setIndicationFilter(int32_t serial, int32_t indFilter) {
    LOG_CALL << serial;
    mHal1_5->setIndicationFilter_1_5(serial, toHidlBitfield<V1_5::IndicationFilter>(indFilter));
    return ok();
}

ScopedAStatus RadioNetwork::setLinkCapacityReportingCriteria(  //
        int32_t serial, int32_t hysteresisMs, int32_t hysteresisDlKbps, int32_t hysteresisUlKbps,
        const std::vector<int32_t>& thrDownlinkKbps, const std::vector<int32_t>& thrUplinkKbps,
        AccessNetwork accessNetwork) {
    LOG_CALL << serial;
    mHal1_5->setLinkCapacityReportingCriteria_1_5(  //
            serial, hysteresisMs, hysteresisDlKbps, hysteresisUlKbps, thrDownlinkKbps,
            thrUplinkKbps, V1_5::AccessNetwork(accessNetwork));
    return ok();
}

ScopedAStatus RadioNetwork::setLocationUpdates(int32_t serial, bool enable) {
    LOG_CALL << serial;
    mHal1_5->setLocationUpdates(serial, enable);
    return ok();
}

ScopedAStatus RadioNetwork::setNetworkSelectionModeAutomatic(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->setNetworkSelectionModeAutomatic(serial);
    return ok();
}

ScopedAStatus RadioNetwork::setNetworkSelectionModeManual(  //
        int32_t serial, const std::string& opNumeric, AccessNetwork ran) {
    LOG_CALL << serial;
    mHal1_5->setNetworkSelectionModeManual_1_5(serial, opNumeric, toRadioAccessNetworks(ran));
    return ok();
}

ScopedAStatus RadioNetwork::setNrDualConnectivityState(int32_t serial,
                                                       aidl::NrDualConnectivityState st) {
    LOG_CALL << serial;
    if (mHal1_6) {
        mHal1_6->setNrDualConnectivityState(serial, V1_6::NrDualConnectivityState(st));
    } else {
        respond()->setNrDualConnectivityStateResponse(notSupported(serial));
    }
    return ok();
}

ScopedAStatus RadioNetwork::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioNetworkResponse>& response,
        const std::shared_ptr<aidl::IRadioNetworkIndication>& indication) {
    LOG_CALL << response << ' ' << indication;
    mCallbackManager->setResponseFunctions(response, indication);
    return ok();
}

ScopedAStatus RadioNetwork::setSignalStrengthReportingCriteria(
        int32_t serial, const std::vector<aidl::SignalThresholdInfo>& infos) {
    LOG_CALL << serial;
    if (infos.size() == 0) {
        LOG(ERROR) << "Threshold info array is empty - dropping setSignalStrengthReportingCriteria";
        return ok();
    }
    if (infos.size() > 1) {
        LOG(WARNING) << "Multi-element reporting criteria are not supported with HIDL HAL";
    }
    if (infos[0].signalMeasurement == aidl::SignalThresholdInfo::SIGNAL_MEASUREMENT_TYPE_ECNO) {
        LOG(WARNING) << "SIGNAL_MEASUREMENT_TYPE_ECNO are not supported with HIDL HAL";
        respond()->setSignalStrengthReportingCriteriaResponse(notSupported(serial));
        return ok();
    }
    mHal1_5->setSignalStrengthReportingCriteria_1_5(serial, toHidl(infos[0]),
                                                    V1_5::AccessNetwork(infos[0].ran));
    return ok();
}

ScopedAStatus RadioNetwork::setSuppServiceNotifications(int32_t serial, bool enable) {
    LOG_CALL << serial;
    mHal1_5->setSuppServiceNotifications(serial, enable);
    return ok();
}

ScopedAStatus RadioNetwork::setSystemSelectionChannels(  //
        int32_t serial, bool specifyCh, const std::vector<aidl::RadioAccessSpecifier>& specifiers) {
    LOG_CALL << serial;
    mHal1_5->setSystemSelectionChannels_1_5(serial, specifyCh, toHidl(specifiers));
    return ok();
}

ScopedAStatus RadioNetwork::startNetworkScan(int32_t serial, const aidl::NetworkScanRequest& req) {
    LOG_CALL << serial;
    mHal1_5->startNetworkScan_1_5(serial, toHidl(req));
    return ok();
}

ScopedAStatus RadioNetwork::stopNetworkScan(int32_t serial) {
    LOG_CALL << serial;
    mHal1_5->stopNetworkScan(serial);
    return ok();
}

ScopedAStatus RadioNetwork::supplyNetworkDepersonalization(int32_t ser, const std::string& nPin) {
    LOG_CALL << ser;
    mHal1_5->supplyNetworkDepersonalization(ser, nPin);
    return ok();
}

ScopedAStatus RadioNetwork::setUsageSetting(int32_t serial, aidl::UsageSetting) {
    LOG_CALL << serial;
    LOG(ERROR) << "setUsageSetting is unsupported by HIDL HALs";
    respond()->setUsageSettingResponse(notSupported(serial));
    return ok();
}

ScopedAStatus RadioNetwork::getUsageSetting(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << "getUsageSetting is unsupported by HIDL HALs";
    respond()->getUsageSettingResponse(notSupported(serial), {});  // {} = neither voice nor data
    return ok();
}

ScopedAStatus RadioNetwork::setEmergencyMode(int32_t serial, aidl::EmergencyMode) {
    LOG_CALL << serial;
    LOG(ERROR) << " setEmergencyMode is unsupported by HIDL HALs";
    respond()->setEmergencyModeResponse(notSupported(serial), {});
    return ok();
}

ScopedAStatus RadioNetwork::triggerEmergencyNetworkScan(int32_t serial,
        const aidl::EmergencyNetworkScanTrigger&) {
    LOG_CALL << serial;
    LOG(ERROR) << " triggerEmergencyNetworkScan is unsupported by HIDL HALs";
    respond()->triggerEmergencyNetworkScanResponse(notSupported(serial));
    return ok();
}

ScopedAStatus RadioNetwork::cancelEmergencyNetworkScan(int32_t serial, bool) {
    LOG_CALL << serial;
    LOG(ERROR) << " cancelEmergencyNetworkScan is unsupported by HIDL HALs";
    respond()->cancelEmergencyNetworkScanResponse(notSupported(serial));
    return ok();
}

ScopedAStatus RadioNetwork::exitEmergencyMode(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " exitEmergencyMode is unsupported by HIDL HALs";
    respond()->exitEmergencyModeResponse(notSupported(serial));
    return ok();
}

ScopedAStatus RadioNetwork::setNullCipherAndIntegrityEnabled(int32_t serial, bool) {
    LOG_CALL << serial;
    LOG(ERROR) << " setNullCipherAndIntegrityEnabled is unsupported by HIDL HALs";
    respond()->setNullCipherAndIntegrityEnabledResponse(notSupported(serial));
    return ok();
}

ScopedAStatus RadioNetwork::isNullCipherAndIntegrityEnabled(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " isNullCipherAndIntegrityEnabled is unsupported by HIDL HALs";
    respond()->isNullCipherAndIntegrityEnabledResponse(notSupported(serial), true);
    return ok();
}

ScopedAStatus RadioNetwork::isN1ModeEnabled(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " isN1ModeEnabled is unsupported by HIDL HALs";
    respond()->isN1ModeEnabledResponse(notSupported(serial), false);
    return ok();
}

ScopedAStatus RadioNetwork::setN1ModeEnabled(int32_t serial, bool /*enable*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " setN1ModeEnabled is unsupported by HIDL HALs";
    respond()->setN1ModeEnabledResponse(notSupported(serial));
    return ok();
}

ScopedAStatus RadioNetwork::isCellularIdentifierTransparencyEnabled(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " isCellularIdentifierTransparencyEnabled is unsupported by HIDL HALs";
    respond()->isCellularIdentifierTransparencyEnabledResponse(notSupported(serial), false);
    return ok();
}

ScopedAStatus RadioNetwork::setCellularIdentifierTransparencyEnabled(int32_t serial,
                                                                     bool /*enabled*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " setCellularIdentifierTransparencyEnabled is unsupported by HIDL HALs";
    respond()->setCellularIdentifierTransparencyEnabledResponse(notSupported(serial));
    return ok();
}

ScopedAStatus RadioNetwork::isSecurityAlgorithmsUpdatedEnabled(int32_t serial) {
    LOG_CALL << serial;
    LOG(ERROR) << " isSecurityAlgorithmsUpdatedEnabled is unsupported by HIDL HALs";
    respond()->isSecurityAlgorithmsUpdatedEnabledResponse(notSupported(serial), false);
    return ok();
}

ScopedAStatus RadioNetwork::setSecurityAlgorithmsUpdatedEnabled(int32_t serial, bool /*enable*/) {
    LOG_CALL << serial;
    LOG(ERROR) << " setSecurityAlgorithmsUpdatedEnabled is unsupported by HIDL HALs";
    respond()->setSecurityAlgorithmsUpdatedEnabledResponse(notSupported(serial));
    return ok();
}

}  // namespace android::hardware::radio::compat
