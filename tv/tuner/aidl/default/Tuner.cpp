/*
 * Copyright 2021 The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "android.hardware.tv.tuner-service.example-Tuner"

#include <aidl/android/hardware/tv/tuner/DemuxFilterMainType.h>
#include <aidl/android/hardware/tv/tuner/Result.h>
#include <utils/Log.h>

#include "Demux.h"
#include "Descrambler.h"
#include "Frontend.h"
#include "Lnb.h"
#include "Tuner.h"

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

Tuner::Tuner() {}

void Tuner::init() {
    // Static Frontends array to maintain local frontends information
    // Array index matches their FrontendId in the default impl
    mFrontendSize = 11;
    mFrontends[0] = ndk::SharedRefBase::make<Frontend>(FrontendType::ISDBS, 0);
    mFrontends[1] = ndk::SharedRefBase::make<Frontend>(FrontendType::ATSC3, 1);
    mFrontends[2] = ndk::SharedRefBase::make<Frontend>(FrontendType::DVBC, 2);
    mFrontends[3] = ndk::SharedRefBase::make<Frontend>(FrontendType::DVBS, 3);
    mFrontends[4] = ndk::SharedRefBase::make<Frontend>(FrontendType::DVBT, 4);
    mFrontends[5] = ndk::SharedRefBase::make<Frontend>(FrontendType::ISDBT, 5);
    mFrontends[6] = ndk::SharedRefBase::make<Frontend>(FrontendType::ANALOG, 6);
    mFrontends[7] = ndk::SharedRefBase::make<Frontend>(FrontendType::ATSC, 7);
    mFrontends[8] = ndk::SharedRefBase::make<Frontend>(FrontendType::ISDBS3, 8);
    mFrontends[9] = ndk::SharedRefBase::make<Frontend>(FrontendType::DTMB, 9);
    mFrontends[10] = ndk::SharedRefBase::make<Frontend>(FrontendType::IPTV, 10);

    mMaxUsableFrontends[FrontendType::ISDBS] = 1;
    mMaxUsableFrontends[FrontendType::ATSC3] = 1;
    mMaxUsableFrontends[FrontendType::DVBC] = 1;
    mMaxUsableFrontends[FrontendType::DVBS] = 1;
    mMaxUsableFrontends[FrontendType::DVBT] = 1;
    mMaxUsableFrontends[FrontendType::ISDBT] = 1;
    mMaxUsableFrontends[FrontendType::ANALOG] = 1;
    mMaxUsableFrontends[FrontendType::ATSC] = 1;
    mMaxUsableFrontends[FrontendType::ISDBS3] = 1;
    mMaxUsableFrontends[FrontendType::DTMB] = 1;
    mMaxUsableFrontends[FrontendType::IPTV] = 1;

    mDemuxes[0] =
            ndk::SharedRefBase::make<Demux>(0, (static_cast<int32_t>(DemuxFilterMainType::TS) |
                                                static_cast<int32_t>(DemuxFilterMainType::MMTP) |
                                                static_cast<int32_t>(DemuxFilterMainType::TLV)));
    mDemuxes[1] =
            ndk::SharedRefBase::make<Demux>(1, (static_cast<int32_t>(DemuxFilterMainType::MMTP) |
                                                static_cast<int32_t>(DemuxFilterMainType::TLV)));
    mDemuxes[2] = ndk::SharedRefBase::make<Demux>(2, static_cast<int32_t>(DemuxFilterMainType::IP));
    mDemuxes[3] = ndk::SharedRefBase::make<Demux>(3, static_cast<int32_t>(DemuxFilterMainType::TS));

    mLnbs.resize(2);
    mLnbs[0] = ndk::SharedRefBase::make<Lnb>(0);
    mLnbs[1] = ndk::SharedRefBase::make<Lnb>(1);
}

Tuner::~Tuner() {}

::ndk::ScopedAStatus Tuner::getFrontendIds(std::vector<int32_t>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    _aidl_return->resize(mFrontendSize);
    for (int i = 0; i < mFrontendSize; i++) {
        (*_aidl_return)[i] = mFrontends[i]->getFrontendId();
    }

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Tuner::getDemuxInfo(int32_t in_demuxId, DemuxInfo* _aidl_return) {
    if (mDemuxes.find(in_demuxId) == mDemuxes.end()) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    } else {
        mDemuxes[in_demuxId]->getDemuxInfo(_aidl_return);
        return ::ndk::ScopedAStatus::ok();
    }
}

::ndk::ScopedAStatus Tuner::getDemuxIds(std::vector<int32_t>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    int numOfDemuxes = mDemuxes.size();
    _aidl_return->resize(numOfDemuxes);
    int i = 0;
    for (auto e = mDemuxes.begin(); e != mDemuxes.end(); e++) {
        (*_aidl_return)[i++] = e->first;
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Tuner::openFrontendById(int32_t in_frontendId,
                                             std::shared_ptr<IFrontend>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    if (in_frontendId >= mFrontendSize || in_frontendId < 0) {
        ALOGW("[   WARN   ] Frontend with id %d isn't available", in_frontendId);
        *_aidl_return = nullptr;
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    mFrontends[in_frontendId]->setTunerService(this->ref<Tuner>());
    *_aidl_return = mFrontends[in_frontendId];
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Tuner::openDemuxById(int32_t in_demuxId,
                                          std::shared_ptr<IDemux>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    if (mDemuxes.find(in_demuxId) == mDemuxes.end()) {
        ALOGW("[   WARN   ] Demux with id %d isn't available", in_demuxId);
        *_aidl_return = nullptr;
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    if (mDemuxes[in_demuxId]->isInUse()) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::UNAVAILABLE));
    } else {
        mDemuxes[in_demuxId]->setTunerService(this->ref<Tuner>());
        mDemuxes[in_demuxId]->setInUse(true);

        *_aidl_return = mDemuxes[in_demuxId];
        return ::ndk::ScopedAStatus::ok();
    }
}

::ndk::ScopedAStatus Tuner::openDemux(std::vector<int32_t>* out_demuxId,
                                      std::shared_ptr<IDemux>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    bool found = false;
    int32_t demuxId = 0;
    for (auto e = mDemuxes.begin(); e != mDemuxes.end(); e++) {
        if (!e->second->isInUse()) {
            found = true;
            demuxId = e->second->getDemuxId();
        }
    }

    if (found) {
        out_demuxId->push_back(demuxId);
        return openDemuxById(demuxId, _aidl_return);
    } else {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::UNAVAILABLE));
    }
}

::ndk::ScopedAStatus Tuner::getDemuxCaps(DemuxCapabilities* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    // IP filter can be an MMTP filter's data source.
    _aidl_return->linkCaps = {0x00, 0x00, 0x02, 0x00, 0x00};
    // Support time filter testing
    _aidl_return->bTimeFilter = true;

    // set filterCaps as the bitwize OR of all the demux' caps
    std::vector<int32_t> demuxIds;
    getDemuxIds(&demuxIds);
    int32_t filterCaps = 0;

    for (int i = 0; i < demuxIds.size(); i++) {
        DemuxInfo demuxInfo;
        getDemuxInfo(demuxIds[i], &demuxInfo);
        filterCaps |= demuxInfo.filterTypes;
    }
    _aidl_return->filterCaps = filterCaps;

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Tuner::openDescrambler(std::shared_ptr<IDescrambler>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    *_aidl_return = ndk::SharedRefBase::make<Descrambler>();

    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Tuner::getFrontendInfo(int32_t in_frontendId, FrontendInfo* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    if (in_frontendId < 0 || in_frontendId >= mFrontendSize) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    mFrontends[in_frontendId]->getFrontendInfo(_aidl_return);
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Tuner::getLnbIds(std::vector<int32_t>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    _aidl_return->resize(mLnbs.size());
    for (int i = 0; i < mLnbs.size(); i++) {
        (*_aidl_return)[i] = mLnbs[i]->getId();
    }

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Tuner::openLnbById(int32_t in_lnbId, std::shared_ptr<ILnb>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    if (in_lnbId >= mLnbs.size()) {
        *_aidl_return = nullptr;
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }

    *_aidl_return = mLnbs[in_lnbId];
    return ::ndk::ScopedAStatus::ok();
}

std::shared_ptr<Frontend> Tuner::getFrontendById(int32_t frontendId) {
    ALOGV("%s", __FUNCTION__);

    return mFrontends[frontendId];
}

::ndk::ScopedAStatus Tuner::openLnbByName(const std::string& /* in_lnbName */,
                                          std::vector<int32_t>* out_lnbId,
                                          std::shared_ptr<ILnb>* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    out_lnbId->push_back(1234);
    *_aidl_return = ndk::SharedRefBase::make<Lnb>();

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Tuner::setLna(bool /* in_bEnable */) {
    ALOGV("%s", __FUNCTION__);

    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Tuner::setMaxNumberOfFrontends(FrontendType in_frontendType,
                                                    int32_t in_maxNumber) {
    ALOGV("%s", __FUNCTION__);

    // In the default implementation, every type only has one frontend.
    if (in_maxNumber < 0 || in_maxNumber > 1) {
        return ::ndk::ScopedAStatus::fromServiceSpecificError(
                static_cast<int32_t>(Result::INVALID_ARGUMENT));
    }
    mMaxUsableFrontends[in_frontendType] = in_maxNumber;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Tuner::getMaxNumberOfFrontends(FrontendType in_frontendType,
                                                    int32_t* _aidl_return) {
    *_aidl_return = mMaxUsableFrontends[in_frontendType];
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Tuner::isLnaSupported(bool* _aidl_return) {
    ALOGV("%s", __FUNCTION__);

    *_aidl_return = true;
    return ::ndk::ScopedAStatus::ok();
}

binder_status_t Tuner::dump(int fd, const char** args, uint32_t numArgs) {
    ALOGV("%s", __FUNCTION__);
    {
        dprintf(fd, "Frontends:\n");
        for (int i = 0; i < mFrontendSize; i++) {
            mFrontends[i]->dump(fd, args, numArgs);
        }
    }
    {
        dprintf(fd, "Demuxs:\n");
        map<int32_t, std::shared_ptr<Demux>>::iterator it;
        for (it = mDemuxes.begin(); it != mDemuxes.end(); it++) {
            it->second->dump(fd, args, numArgs);
        }
    }
    {
        dprintf(fd, "Lnbs:\n");
        for (int i = 0; i < mLnbs.size(); i++) {
            mLnbs[i]->dump(fd, args, numArgs);
        }
    }
    return STATUS_OK;
}

void Tuner::setFrontendAsDemuxSource(int32_t frontendId, int32_t demuxId) {
    mFrontendToDemux[frontendId] = demuxId;
    if (mFrontends[frontendId] != nullptr && mFrontends[frontendId]->isLocked()) {
        mDemuxes[demuxId]->startFrontendInputLoop();
    }
}

void Tuner::removeDemux(int32_t demuxId) {
    map<int32_t, int32_t>::iterator it;
    for (it = mFrontendToDemux.begin(); it != mFrontendToDemux.end(); it++) {
        if (it->second == demuxId) {
            it = mFrontendToDemux.erase(it);
            break;
        }
    }
    mDemuxes[demuxId]->setInUse(false);
}

void Tuner::removeFrontend(int32_t frontendId) {
    map<int32_t, int32_t>::iterator it = mFrontendToDemux.find(frontendId);
    if (it != mFrontendToDemux.end()) {
        mDemuxes[it->second]->setInUse(false);
    }
    mFrontendToDemux.erase(frontendId);
}

void Tuner::frontendStopTune(int32_t frontendId) {
    map<int32_t, int32_t>::iterator it = mFrontendToDemux.find(frontendId);
    int32_t demuxId;
    if (it != mFrontendToDemux.end()) {
        demuxId = it->second;
        mDemuxes[demuxId]->stopFrontendInput();
    }
}

void Tuner::frontendStartTune(int32_t frontendId) {
    map<int32_t, int32_t>::iterator it = mFrontendToDemux.find(frontendId);
    int32_t demuxId;
    if (it != mFrontendToDemux.end()) {
        demuxId = it->second;
        mDemuxes[demuxId]->startFrontendInputLoop();
    }
}

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
