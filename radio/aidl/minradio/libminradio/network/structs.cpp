/*
 * Copyright (C) 2024 The Android Open Source Project
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
#include <libminradio/network/structs.h>

#include <android-base/logging.h>
#include <libminradio/binder_printing.h>

namespace android::hardware::radio::minimal::structs {

using namespace ::android::hardware::radio::minimal::binder_printing;
using ::aidl::android::hardware::radio::RadioConst;
namespace aidl = ::aidl::android::hardware::radio::network;

aidl::SignalStrength makeSignalStrength() {
    constexpr aidl::GsmSignalStrength gsm{
            RadioConst::VALUE_UNAVAILABLE,
            RadioConst::VALUE_UNAVAILABLE,
            RadioConst::VALUE_UNAVAILABLE,
    };
    constexpr aidl::LteSignalStrength lte{
            RadioConst::VALUE_UNAVAILABLE, RadioConst::VALUE_UNAVAILABLE,
            RadioConst::VALUE_UNAVAILABLE, RadioConst::VALUE_UNAVAILABLE,
            RadioConst::VALUE_UNAVAILABLE, RadioConst::VALUE_UNAVAILABLE,
            RadioConst::VALUE_UNAVAILABLE,
    };
    constexpr aidl::TdscdmaSignalStrength tdscdma{
            RadioConst::VALUE_UNAVAILABLE,
            RadioConst::VALUE_UNAVAILABLE,
            RadioConst::VALUE_UNAVAILABLE,
    };
    constexpr aidl::WcdmaSignalStrength wcdma{
            RadioConst::VALUE_UNAVAILABLE,
            RadioConst::VALUE_UNAVAILABLE,
            RadioConst::VALUE_UNAVAILABLE,
            RadioConst::VALUE_UNAVAILABLE,
    };
    constexpr aidl::NrSignalStrength nr{
            RadioConst::VALUE_UNAVAILABLE, RadioConst::VALUE_UNAVAILABLE,
            RadioConst::VALUE_UNAVAILABLE, RadioConst::VALUE_UNAVAILABLE,
            RadioConst::VALUE_UNAVAILABLE, RadioConst::VALUE_UNAVAILABLE,
            RadioConst::VALUE_UNAVAILABLE, {},
            RadioConst::VALUE_UNAVAILABLE,
    };

    return {
            .gsm = gsm,
            .lte = lte,
            .tdscdma = tdscdma,
            .wcdma = wcdma,
            .nr = nr,
    };
}

aidl::CellInfo makeCellInfo(const aidl::RegStateResult& regState,
                            const aidl::SignalStrength& signalStrength) {
    std::optional<aidl::CellInfoRatSpecificInfo> ratSpecificInfo;
    auto& cellId = regState.cellIdentity;
    switch (cellId.getTag()) {
        case aidl::CellIdentity::Tag::noinit:
            break;
        case aidl::CellIdentity::Tag::gsm:
            ratSpecificInfo = aidl::CellInfoGsm{
                    .cellIdentityGsm = cellId.get<aidl::CellIdentity::Tag::gsm>(),
                    .signalStrengthGsm = signalStrength.gsm,
            };
            break;
        case aidl::CellIdentity::Tag::wcdma:
            ratSpecificInfo = aidl::CellInfoWcdma{
                    .cellIdentityWcdma = cellId.get<aidl::CellIdentity::Tag::wcdma>(),
                    .signalStrengthWcdma = signalStrength.wcdma,
            };
            break;
        case aidl::CellIdentity::Tag::tdscdma:
            ratSpecificInfo = aidl::CellInfoTdscdma{
                    .cellIdentityTdscdma = cellId.get<aidl::CellIdentity::Tag::tdscdma>(),
                    .signalStrengthTdscdma = signalStrength.tdscdma,
            };
            break;
        case aidl::CellIdentity::Tag::lte:
            ratSpecificInfo = aidl::CellInfoLte{
                    .cellIdentityLte = cellId.get<aidl::CellIdentity::Tag::lte>(),
                    .signalStrengthLte = signalStrength.lte,
            };
            break;
        case aidl::CellIdentity::Tag::nr:
            ratSpecificInfo = aidl::CellInfoNr{
                    .cellIdentityNr = cellId.get<aidl::CellIdentity::Tag::nr>(),
                    .signalStrengthNr = signalStrength.nr,
            };
            break;
    }
    CHECK(ratSpecificInfo.has_value()) << "Cell identity not handled: " << cellId;

    bool isRegistered = regState.regState == aidl::RegState::REG_HOME ||
                        regState.regState == aidl::RegState::REG_ROAMING;

    return aidl::CellInfo{
            .registered = isRegistered,
            .connectionStatus = isRegistered ? aidl::CellConnectionStatus::PRIMARY_SERVING
                                             : aidl::CellConnectionStatus::NONE,
            .ratSpecificInfo = *ratSpecificInfo,
    };
}

aidl::OperatorInfo getOperatorInfo(const aidl::CellIdentity& cellId) {
    switch (cellId.getTag()) {
        case aidl::CellIdentity::Tag::noinit:
            return {};
        case aidl::CellIdentity::Tag::gsm:
            return cellId.get<aidl::CellIdentity::Tag::gsm>().operatorNames;
        case aidl::CellIdentity::Tag::wcdma:
            return cellId.get<aidl::CellIdentity::Tag::wcdma>().operatorNames;
        case aidl::CellIdentity::Tag::tdscdma:
            return cellId.get<aidl::CellIdentity::Tag::tdscdma>().operatorNames;
        case aidl::CellIdentity::Tag::lte:
            return cellId.get<aidl::CellIdentity::Tag::lte>().operatorNames;
        case aidl::CellIdentity::Tag::nr:
            return cellId.get<aidl::CellIdentity::Tag::nr>().operatorNames;
    }
    LOG(FATAL) << "Cell identity not handled: " << cellId;
}

int32_t rssiToSignalStrength(int32_t rssi) {
    // 3GPP TS 27.007 8.5
    if (rssi <= -113) return 0;
    if (rssi >= -51) return 31;
    if (rssi >= -1) return 99;
    return (rssi + 113) / 2;
}

int32_t validateRsrp(int32_t rsrp) {
    // 3GPP TS 27.007 8.69
    if (rsrp < -140 || rsrp > -44) return 140;  // CellInfoTest: RSRP Must be valid for LTE
    return -rsrp;
}

int32_t validateRsrq(int32_t rsrq) {
    // 3GPP TS 27.007 8.69
    if (rsrq < -20 || rsrq > -3) return RadioConst::VALUE_UNAVAILABLE;
    return -rsrq;
}

static bool validateSignalThresholdInfo(const aidl::SignalThresholdInfo& info) {
    if (info.signalMeasurement <= 0) return false;
    if (info.hysteresisMs < 0) return false;
    if (info.hysteresisDb != 0 && info.thresholds.size() > 1) {
        int minThreshold = info.thresholds[1] - info.thresholds[0];
        for (size_t i = 2; i < info.thresholds.size(); i++) {
            int delta = info.thresholds[i] - info.thresholds[i - 1];
            if (minThreshold < delta) minThreshold = delta;
        }
        if (minThreshold < 0) return false;
        if (info.hysteresisDb > minThreshold) return false;
    }
    return true;
}

bool validateSignalThresholdInfos(const std::vector<aidl::SignalThresholdInfo>& infos) {
    for (auto& info : infos) {
        if (!validateSignalThresholdInfo(info)) return false;
    }
    return true;
}

}  // namespace android::hardware::radio::minimal::structs
