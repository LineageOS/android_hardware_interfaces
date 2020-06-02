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

#include <android-base/logging.h>
#include <android/hardware/cas/1.0/types.h>
#include <android/hardware/cas/1.2/ICas.h>
#include <android/hardware/cas/1.2/ICasListener.h>
#include <android/hardware/cas/1.2/IMediaCasService.h>
#include <android/hardware/cas/1.2/types.h>
#include <android/hardware/tv/tuner/1.0/IDescrambler.h>
#include <android/hardware/tv/tuner/1.0/IDvr.h>
#include <android/hardware/tv/tuner/1.0/IDvrCallback.h>
#include <android/hardware/tv/tuner/1.0/ITuner.h>
#include <android/hardware/tv/tuner/1.0/types.h>
#include <fmq/MessageQueue.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/Status.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <fstream>
#include <iostream>
#include <map>

using android::Condition;
using android::Mutex;
using android::sp;
using android::hardware::EventFlag;
using android::hardware::hidl_handle;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::kSynchronizedReadWrite;
using android::hardware::MessageQueue;
using android::hardware::MQDescriptorSync;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::cas::V1_2::ICas;
using android::hardware::cas::V1_2::ICasListener;
using android::hardware::cas::V1_2::IMediaCasService;
using android::hardware::cas::V1_2::ScramblingMode;
using android::hardware::cas::V1_2::SessionIntent;
using android::hardware::cas::V1_2::Status;
using android::hardware::cas::V1_2::StatusEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterMainType;
using android::hardware::tv::tuner::V1_0::DemuxFilterSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterStatus;
using android::hardware::tv::tuner::V1_0::DemuxFilterType;
using android::hardware::tv::tuner::V1_0::DemuxMmtpFilterType;
using android::hardware::tv::tuner::V1_0::DemuxPid;
using android::hardware::tv::tuner::V1_0::DemuxTsFilterType;
using android::hardware::tv::tuner::V1_0::IDescrambler;
using android::hardware::tv::tuner::V1_0::IFilter;
using android::hardware::tv::tuner::V1_0::ITuner;
using android::hardware::tv::tuner::V1_0::Result;
using android::hardware::tv::tuner::V1_0::TunerKeyToken;

using ::testing::AssertionResult;

using namespace std;

class MediaCasListener : public ICasListener {
  public:
    virtual Return<void> onEvent(int32_t /*event*/, int32_t /*arg*/,
                                 const hidl_vec<uint8_t>& /*data*/) override {
        return Void();
    }

    virtual Return<void> onSessionEvent(const hidl_vec<uint8_t>& /*sessionId*/, int32_t /*event*/,
                                        int32_t /*arg*/,
                                        const hidl_vec<uint8_t>& /*data*/) override {
        return Void();
    }

    virtual Return<void> onStatusUpdate(StatusEvent /*event*/, int32_t /*arg*/) override {
        return Void();
    }
};

class DescramblerTests {
  public:
    void setService(sp<ITuner> tuner) { mService = tuner; }
    void setCasService(sp<IMediaCasService> casService) { mMediaCasService = casService; }

    AssertionResult setKeyToken(TunerKeyToken token);
    AssertionResult openDescrambler(uint32_t demuxId);
    AssertionResult addPid(DemuxPid pid, sp<IFilter> optionalSourceFilter);
    AssertionResult removePid(DemuxPid pid, sp<IFilter> optionalSourceFilter);
    AssertionResult closeDescrambler();
    AssertionResult getKeyToken(int32_t caSystemId, string provisonStr,
                                hidl_vec<uint8_t> hidlPvtData, TunerKeyToken& token);
    AssertionResult getDemuxPidFromFilterSettings(DemuxFilterType type,
                                                  DemuxFilterSettings settings, DemuxPid& pid);

  protected:
    static AssertionResult failure() { return ::testing::AssertionFailure(); }

    static AssertionResult success() { return ::testing::AssertionSuccess(); }

    sp<ITuner> mService;
    sp<ICas> mCas;
    sp<IMediaCasService> mMediaCasService;
    sp<MediaCasListener> mCasListener;
    sp<IDescrambler> mDescrambler;

  private:
    AssertionResult openCasSession(TunerKeyToken& sessionId, vector<uint8_t> hidlPvtData);
    AssertionResult createCasPlugin(int32_t caSystemId);
};
