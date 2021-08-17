/*
 * Copyright 2020 The Android Open Source Project
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

#define LOG_TAG "android.hardware.tv.tuner@1.1-Tuner"

#include "Tuner.h"
#include <utils/Log.h>
#include "Demux.h"
#include "Descrambler.h"
#include "Frontend.h"
#include "Lnb.h"

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

Tuner::Tuner() {
    // Static Frontends array to maintain local frontends information
    // Array index matches their FrontendId in the default impl
    mFrontendSize = 10;
    mFrontends[0] = new Frontend(FrontendType::ISDBS, 0, this);
    mFrontends[1] = new Frontend(FrontendType::ATSC3, 1, this);
    mFrontends[2] = new Frontend(FrontendType::DVBC, 2, this);
    mFrontends[3] = new Frontend(FrontendType::DVBS, 3, this);
    mFrontends[4] = new Frontend(FrontendType::DVBT, 4, this);
    mFrontends[5] = new Frontend(FrontendType::ISDBT, 5, this);
    mFrontends[6] = new Frontend(FrontendType::ANALOG, 6, this);
    mFrontends[7] = new Frontend(FrontendType::ATSC, 7, this);
    mFrontends[8] = new Frontend(FrontendType::ISDBS3, 8, this);
    mFrontends[9] =
            new Frontend(static_cast<V1_0::FrontendType>(V1_1::FrontendType::DTMB), 9, this);

    FrontendInfo::FrontendCapabilities caps;
    vector<FrontendStatusType> statusCaps;

    caps = FrontendInfo::FrontendCapabilities();
    caps.isdbsCaps(FrontendIsdbsCapabilities());
    mFrontendCaps[0] = caps;
    statusCaps = {
            FrontendStatusType::DEMOD_LOCK,
            FrontendStatusType::SNR,
            FrontendStatusType::FEC,
            FrontendStatusType::MODULATION,
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::MODULATIONS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::ROLL_OFF),
    };
    mFrontendStatusCaps[0] = statusCaps;

    caps = FrontendInfo::FrontendCapabilities();
    caps.atsc3Caps(FrontendAtsc3Capabilities());
    mFrontendCaps[1] = caps;
    statusCaps = {
            FrontendStatusType::BER,
            FrontendStatusType::PER,
            FrontendStatusType::ATSC3_PLP_INFO,
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::MODULATIONS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::BERS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::INTERLEAVINGS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::BANDWIDTH),
    };
    mFrontendStatusCaps[1] = statusCaps;

    caps = FrontendInfo::FrontendCapabilities();
    caps.dvbcCaps(FrontendDvbcCapabilities());
    mFrontendCaps[2] = caps;
    statusCaps = {
            FrontendStatusType::PRE_BER,
            FrontendStatusType::SIGNAL_QUALITY,
            FrontendStatusType::MODULATION,
            FrontendStatusType::SPECTRAL,
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::MODULATIONS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::CODERATES),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::INTERLEAVINGS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::BANDWIDTH),
    };
    mFrontendStatusCaps[2] = statusCaps;

    caps = FrontendInfo::FrontendCapabilities();
    caps.dvbsCaps(FrontendDvbsCapabilities());
    mFrontendCaps[3] = caps;
    statusCaps = {
            FrontendStatusType::SIGNAL_STRENGTH,
            FrontendStatusType::SYMBOL_RATE,
            FrontendStatusType::MODULATION,
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::MODULATIONS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::ROLL_OFF),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::IS_MISO),
    };
    mFrontendStatusCaps[3] = statusCaps;

    caps = FrontendInfo::FrontendCapabilities();
    caps.dvbtCaps(FrontendDvbtCapabilities());
    mFrontendCaps[4] = caps;
    statusCaps = {
            FrontendStatusType::EWBS,
            FrontendStatusType::PLP_ID,
            FrontendStatusType::HIERARCHY,
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::MODULATIONS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::BANDWIDTH),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::GUARD_INTERVAL),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::TRANSMISSION_MODE),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::T2_SYSTEM_ID),
    };
    mFrontendStatusCaps[4] = statusCaps;

    caps = FrontendInfo::FrontendCapabilities();
    FrontendIsdbtCapabilities isdbtCaps{
            .modeCap = FrontendIsdbtMode::MODE_1 | FrontendIsdbtMode::MODE_2,
            .bandwidthCap = (unsigned int)FrontendIsdbtBandwidth::BANDWIDTH_6MHZ,
            .modulationCap = (unsigned int)FrontendIsdbtModulation::MOD_16QAM,
            // ISDBT shares coderate and guard interval with DVBT
            .coderateCap = FrontendDvbtCoderate::CODERATE_4_5 | FrontendDvbtCoderate::CODERATE_6_7,
            .guardIntervalCap = (unsigned int)FrontendDvbtGuardInterval::INTERVAL_1_128,
    };
    caps.isdbtCaps(isdbtCaps);
    mFrontendCaps[5] = caps;
    statusCaps = {
            FrontendStatusType::AGC,
            FrontendStatusType::LNA,
            FrontendStatusType::MODULATION,
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::MODULATIONS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::BANDWIDTH),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::GUARD_INTERVAL),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::TRANSMISSION_MODE),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::ISDBT_SEGMENTS),
    };
    mFrontendStatusCaps[5] = statusCaps;

    caps = FrontendInfo::FrontendCapabilities();
    caps.analogCaps(FrontendAnalogCapabilities());
    mFrontendCaps[6] = caps;
    statusCaps = {
            FrontendStatusType::LAYER_ERROR,
            FrontendStatusType::MER,
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::UEC),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::TS_DATA_RATES),
    };
    mFrontendStatusCaps[6] = statusCaps;

    caps = FrontendInfo::FrontendCapabilities();
    caps.atscCaps(FrontendAtscCapabilities());
    mFrontendCaps[7] = caps;
    statusCaps = {
            FrontendStatusType::FREQ_OFFSET,
            FrontendStatusType::RF_LOCK,
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::MODULATIONS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::IS_LINEAR),
    };
    mFrontendStatusCaps[7] = statusCaps;

    caps = FrontendInfo::FrontendCapabilities();
    caps.isdbs3Caps(FrontendIsdbs3Capabilities());
    mFrontendCaps[8] = caps;
    statusCaps = {
            FrontendStatusType::DEMOD_LOCK,
            FrontendStatusType::MODULATION,
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::MODULATIONS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::ROLL_OFF),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::IS_SHORT_FRAMES),
    };
    mFrontendStatusCaps[8] = statusCaps;

    statusCaps = {
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::MODULATIONS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::INTERLEAVINGS),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::BANDWIDTH),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::GUARD_INTERVAL),
            static_cast<FrontendStatusType>(V1_1::FrontendStatusTypeExt1_1::TRANSMISSION_MODE),
    };
    mFrontendStatusCaps[9] = statusCaps;

    mLnbs.resize(2);
    mLnbs[0] = new Lnb(0);
    mLnbs[1] = new Lnb(1);
}

Tuner::~Tuner() {}

Return<void> Tuner::getFrontendIds(getFrontendIds_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    vector<FrontendId> frontendIds;
    frontendIds.resize(mFrontendSize);
    for (int i = 0; i < mFrontendSize; i++) {
        frontendIds[i] = mFrontends[i]->getFrontendId();
    }

    _hidl_cb(Result::SUCCESS, frontendIds);
    return Void();
}

Return<void> Tuner::openFrontendById(uint32_t frontendId, openFrontendById_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    if (frontendId >= mFrontendSize || frontendId < 0) {
        ALOGW("[   WARN   ] Frontend with id %d isn't available", frontendId);
        _hidl_cb(Result::UNAVAILABLE, nullptr);
        return Void();
    }

    _hidl_cb(Result::SUCCESS, mFrontends[frontendId]);
    return Void();
}

Return<void> Tuner::openDemux(openDemux_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    uint32_t demuxId = mLastUsedId + 1;
    mLastUsedId += 1;
    sp<Demux> demux = new Demux(demuxId, this);
    mDemuxes[demuxId] = demux;

    _hidl_cb(Result::SUCCESS, demuxId, demux);
    return Void();
}

Return<void> Tuner::getDemuxCaps(getDemuxCaps_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    DemuxCapabilities caps;

    // IP filter can be an MMTP filter's data source.
    caps.linkCaps = {0x00, 0x00, 0x02, 0x00, 0x00};
    // Support time filter testing
    caps.bTimeFilter = true;
    _hidl_cb(Result::SUCCESS, caps);
    return Void();
}

Return<void> Tuner::openDescrambler(openDescrambler_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    sp<V1_0::IDescrambler> descrambler = new Descrambler();

    _hidl_cb(Result::SUCCESS, descrambler);
    return Void();
}

Return<void> Tuner::getFrontendInfo(FrontendId frontendId, getFrontendInfo_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    FrontendInfo info;
    if (frontendId >= mFrontendSize) {
        _hidl_cb(Result::INVALID_ARGUMENT, info);
        return Void();
    }

    // assign randomly selected values for testing.
    info = {
            .type = mFrontends[frontendId]->getFrontendType(),
            .minFrequency = 139,
            .maxFrequency = 1139,
            .minSymbolRate = 45,
            .maxSymbolRate = 1145,
            .acquireRange = 30,
            .exclusiveGroupId = 57,
            .statusCaps = mFrontendStatusCaps[frontendId],
            .frontendCaps = mFrontendCaps[frontendId],
    };

    _hidl_cb(Result::SUCCESS, info);
    return Void();
}

Return<void> Tuner::getLnbIds(getLnbIds_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    vector<V1_0::LnbId> lnbIds;
    lnbIds.resize(mLnbs.size());
    for (int i = 0; i < lnbIds.size(); i++) {
        lnbIds[i] = mLnbs[i]->getId();
    }

    _hidl_cb(Result::SUCCESS, lnbIds);
    return Void();
}

Return<void> Tuner::openLnbById(V1_0::LnbId lnbId, openLnbById_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    if (lnbId >= mLnbs.size()) {
        _hidl_cb(Result::INVALID_ARGUMENT, nullptr);
        return Void();
    }

    _hidl_cb(Result::SUCCESS, mLnbs[lnbId]);
    return Void();
}

sp<Frontend> Tuner::getFrontendById(uint32_t frontendId) {
    ALOGV("%s", __FUNCTION__);

    return mFrontends[frontendId];
}

Return<void> Tuner::openLnbByName(const hidl_string& /*lnbName*/, openLnbByName_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    sp<V1_0::ILnb> lnb = new Lnb();

    _hidl_cb(Result::SUCCESS, 1234, lnb);
    return Void();
}

Return<void> Tuner::getFrontendDtmbCapabilities(uint32_t frontendId,
                                                getFrontendDtmbCapabilities_cb _hidl_cb) {
    ALOGV("%s", __FUNCTION__);

    if (mFrontends[frontendId] != nullptr &&
        (mFrontends[frontendId]->getFrontendType() ==
         static_cast<V1_0::FrontendType>(V1_1::FrontendType::DTMB))) {
        _hidl_cb(Result::SUCCESS, mDtmbCaps);
    } else {
        _hidl_cb(Result::UNAVAILABLE, mDtmbCaps);
    }
    return Void();
}

void Tuner::setFrontendAsDemuxSource(uint32_t frontendId, uint32_t demuxId) {
    mFrontendToDemux[frontendId] = demuxId;
    if (mFrontends[frontendId] != nullptr && mFrontends[frontendId]->isLocked()) {
        mDemuxes[demuxId]->startFrontendInputLoop();
    }
}

void Tuner::removeDemux(uint32_t demuxId) {
    map<uint32_t, uint32_t>::iterator it;
    for (it = mFrontendToDemux.begin(); it != mFrontendToDemux.end(); it++) {
        if (it->second == demuxId) {
            it = mFrontendToDemux.erase(it);
            break;
        }
    }
    mDemuxes.erase(demuxId);
}

void Tuner::removeFrontend(uint32_t frontendId) {
    mFrontendToDemux.erase(frontendId);
}

void Tuner::frontendStopTune(uint32_t frontendId) {
    map<uint32_t, uint32_t>::iterator it = mFrontendToDemux.find(frontendId);
    uint32_t demuxId;
    if (it != mFrontendToDemux.end()) {
        demuxId = it->second;
        mDemuxes[demuxId]->stopFrontendInput();
    }
}

void Tuner::frontendStartTune(uint32_t frontendId) {
    map<uint32_t, uint32_t>::iterator it = mFrontendToDemux.find(frontendId);
    uint32_t demuxId;
    if (it != mFrontendToDemux.end()) {
        demuxId = it->second;
        mDemuxes[demuxId]->startFrontendInputLoop();
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
