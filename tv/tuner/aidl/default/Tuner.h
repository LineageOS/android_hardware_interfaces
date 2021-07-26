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

#pragma once

#include <aidl/android/hardware/tv/tuner/BnTuner.h>
#include <aidl/android/hardware/tv/tuner/FrontendCapabilities.h>

#include <map>
#include "Demux.h"
#include "Frontend.h"
#include "Lnb.h"

using namespace std;

namespace aidl {
namespace android {
namespace hardware {
namespace tv {
namespace tuner {

class Frontend;
class Demux;
class Lnb;

class Tuner : public BnTuner {
  public:
    Tuner();
    virtual ~Tuner();

    ::ndk::ScopedAStatus getFrontendIds(std::vector<int32_t>* _aidl_return) override;
    ::ndk::ScopedAStatus openFrontendById(int32_t in_frontendId,
                                          std::shared_ptr<IFrontend>* _aidl_return) override;
    ::ndk::ScopedAStatus openDemux(std::vector<int32_t>* out_demuxId,
                                   std::shared_ptr<IDemux>* _aidl_return) override;
    ::ndk::ScopedAStatus getDemuxCaps(DemuxCapabilities* _aidl_return) override;
    ::ndk::ScopedAStatus openDescrambler(std::shared_ptr<IDescrambler>* _aidl_return) override;
    ::ndk::ScopedAStatus getFrontendInfo(int32_t in_frontendId,
                                         FrontendInfo* _aidl_return) override;
    ::ndk::ScopedAStatus getLnbIds(std::vector<int32_t>* _aidl_return) override;
    ::ndk::ScopedAStatus openLnbById(int32_t in_lnbId,
                                     std::shared_ptr<ILnb>* _aidl_return) override;
    ::ndk::ScopedAStatus openLnbByName(const std::string& in_lnbName,
                                       std::vector<int32_t>* out_lnbId,
                                       std::shared_ptr<ILnb>* _aidl_return) override;

    std::shared_ptr<Frontend> getFrontendById(int32_t frontendId);
    void setFrontendAsDemuxSource(int32_t frontendId, int32_t demuxId);
    void frontendStartTune(int32_t frontendId);
    void frontendStopTune(int32_t frontendId);
    void removeDemux(int32_t demuxId);
    void removeFrontend(int32_t frontendId);

  private:
    // Static mFrontends array to maintain local frontends information
    map<int32_t, std::shared_ptr<Frontend>> mFrontends;
    map<int32_t, FrontendCapabilities> mFrontendCaps;
    map<int32_t, vector<FrontendStatusType>> mFrontendStatusCaps;
    map<int32_t, int32_t> mFrontendToDemux;
    map<int32_t, std::shared_ptr<Demux>> mDemuxes;
    // To maintain how many Frontends we have
    int mFrontendSize;
    // The last used demux id. Initial value is -1.
    // First used id will be 0.
    int32_t mLastUsedId = -1;
    vector<std::shared_ptr<Lnb>> mLnbs;
};

}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android
}  // namespace aidl
