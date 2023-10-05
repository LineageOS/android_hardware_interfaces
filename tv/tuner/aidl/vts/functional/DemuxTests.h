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

#include <aidl/android/hardware/tv/tuner/DemuxCapabilities.h>
#include <aidl/android/hardware/tv/tuner/DemuxInfo.h>
#include <aidl/android/hardware/tv/tuner/IDemux.h>
#include <aidl/android/hardware/tv/tuner/IFilter.h>
#include <aidl/android/hardware/tv/tuner/ITuner.h>

#include <gtest/gtest.h>
#include <log/log.h>

using ::testing::AssertionResult;

using namespace aidl::android::hardware::tv::tuner;

class DemuxTests {
  public:
    void setService(std::shared_ptr<ITuner> tuner) { mService = tuner; }

    AssertionResult getDemuxIds(std::vector<int32_t>& demuxIds);
    AssertionResult openDemux(std::shared_ptr<IDemux>& demux, int32_t& demuxId);
    AssertionResult openDemuxById(int32_t demuxId, std::shared_ptr<IDemux>& demux);
    AssertionResult setDemuxFrontendDataSource(int32_t frontendId);
    AssertionResult getAvSyncId(std::shared_ptr<IFilter> filter, int32_t& avSyncHwId);
    AssertionResult getAvSyncTime(int32_t avSyncId);
    AssertionResult getDemuxCaps(DemuxCapabilities& demuxCaps);
    AssertionResult getDemuxInfo(int32_t demuxId, DemuxInfo& demuxInfo);
    AssertionResult closeDemux();

  protected:
    static AssertionResult failure() { return ::testing::AssertionFailure(); }

    static AssertionResult success() { return ::testing::AssertionSuccess(); }

    std::shared_ptr<ITuner> mService;
    std::shared_ptr<IDemux> mDemux;
};
