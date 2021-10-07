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
#include <android/hardware/tv/tuner/1.0/types.h>
#include <android/hardware/tv/tuner/1.1/IFrontend.h>
#include <android/hardware/tv/tuner/1.1/IFrontendCallback.h>
#include <android/hardware/tv/tuner/1.1/ITuner.h>
#include <binder/MemoryDealer.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/ServiceManagement.h>
#include <hidl/Status.h>
#include <hidlmemory/FrameworkUtils.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <map>

#include "DvrTests.h"
#include "VtsHalTvTunerV1_1TestConfigurations.h"

#define WAIT_TIMEOUT 3000000000
#define INVALID_ID -1

using android::Condition;
using android::IMemory;
using android::IMemoryHeap;
using android::MemoryDealer;
using android::Mutex;
using android::sp;
using android::hardware::fromHeap;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::tv::tuner::V1_0::FrontendEventType;
using android::hardware::tv::tuner::V1_0::FrontendId;
using android::hardware::tv::tuner::V1_0::FrontendInfo;
using android::hardware::tv::tuner::V1_0::FrontendScanMessage;
using android::hardware::tv::tuner::V1_0::FrontendScanMessageType;
using android::hardware::tv::tuner::V1_0::FrontendScanType;
using android::hardware::tv::tuner::V1_0::IFrontend;
using android::hardware::tv::tuner::V1_0::Result;
using android::hardware::tv::tuner::V1_1::FrontendDtmbCapabilities;
using android::hardware::tv::tuner::V1_1::FrontendModulation;
using android::hardware::tv::tuner::V1_1::FrontendScanMessageExt1_1;
using android::hardware::tv::tuner::V1_1::FrontendScanMessageTypeExt1_1;
using android::hardware::tv::tuner::V1_1::IFrontendCallback;
using android::hardware::tv::tuner::V1_1::ITuner;

using ::testing::AssertionResult;

using namespace std;

#define INVALID_ID -1
#define WAIT_TIMEOUT 3000000000

class FrontendCallback : public IFrontendCallback {
  public:
    virtual Return<void> onEvent(FrontendEventType frontendEventType) override;
    virtual Return<void> onScanMessage(FrontendScanMessageType type,
                                       const FrontendScanMessage& message) override;
    virtual Return<void> onScanMessageExt1_1(FrontendScanMessageTypeExt1_1 type,
                                             const FrontendScanMessageExt1_1& message) override;

    void tuneTestOnLock(sp<IFrontend>& frontend, FrontendSettings settings,
                        FrontendSettingsExt1_1 settingsExt1_1);
    void scanTest(sp<IFrontend>& frontend, FrontendConfig1_1 config, FrontendScanType type);

    // Helper methods
    uint32_t getTargetFrequency(FrontendSettings settings);
    void resetBlindScanStartingFrequency(FrontendConfig1_1& config, uint32_t resetingFreq);

  private:
    void readFrontendScanMessageExt1_1Modulation(FrontendModulation modulation);

    bool mEventReceived = false;
    bool mScanMessageReceived = false;
    bool mLockMsgReceived = false;
    bool mScanMsgProcessed = true;
    FrontendScanMessageType mScanMessageType;
    FrontendScanMessage mScanMessage;
    hidl_vec<uint8_t> mEventMessage;
    android::Mutex mMsgLock;
    android::Condition mMsgCondition;
    android::Condition mLockMsgCondition;
};

class FrontendTests {
  public:
    sp<ITuner> mService;

    void setService(sp<ITuner> tuner) {
        mService = tuner;
        mDvrTests.setService(tuner);
        getDefaultSoftwareFrontendPlaybackConfig(mDvrConfig);
    }

    AssertionResult getFrontendIds();
    AssertionResult getFrontendInfo(uint32_t frontendId);
    AssertionResult openFrontendById(uint32_t frontendId);
    AssertionResult setFrontendCallback();
    AssertionResult scanFrontend(FrontendConfig1_1 config, FrontendScanType type);
    AssertionResult stopScanFrontend();
    AssertionResult tuneFrontend(FrontendConfig1_1 config, bool testWithDemux);
    void verifyFrontendStatusExt1_1(vector<FrontendStatusTypeExt1_1> statusTypes,
                                    vector<FrontendStatusExt1_1> expectStatuses);
    AssertionResult stopTuneFrontend(bool testWithDemux);
    AssertionResult closeFrontend();
    AssertionResult getFrontendDtmbCaps(uint32_t);

    AssertionResult linkCiCam(uint32_t ciCamId);
    AssertionResult unlinkCiCam(uint32_t ciCamId);

    void getFrontendIdByType(FrontendType feType, uint32_t& feId);
    void tuneTest(FrontendConfig1_1 frontendConf);
    void scanTest(FrontendConfig1_1 frontend, FrontendScanType type);
    void getFrontendDtmbCapsTest();

    void setDvrTests(DvrTests dvrTests) { mDvrTests = dvrTests; }
    void setDemux(sp<IDemux> demux) { mDvrTests.setDemux(demux); }
    void setSoftwareFrontendDvrConfig(DvrConfig conf) { mDvrConfig = conf; }

  protected:
    static AssertionResult failure() { return ::testing::AssertionFailure(); }
    static AssertionResult success() { return ::testing::AssertionSuccess(); }

    void getDefaultSoftwareFrontendPlaybackConfig(DvrConfig& dvrConfig) {
        PlaybackSettings playbackSettings{
                .statusMask = 0xf,
                .lowThreshold = 0x1000,
                .highThreshold = 0x07fff,
                .dataFormat = DataFormat::ES,
                .packetSize = 188,
        };
        dvrConfig.type = DvrType::PLAYBACK;
        dvrConfig.playbackInputFile = "/data/local/tmp/test.es";
        dvrConfig.bufferSize = FMQ_SIZE_4M;
        dvrConfig.settings.playback(playbackSettings);
    }

    sp<IFrontend> mFrontend;
    FrontendInfo mFrontendInfo;
    sp<FrontendCallback> mFrontendCallback;
    hidl_vec<FrontendId> mFeIds;

    DvrTests mDvrTests;
    bool mIsSoftwareFe = false;
    DvrConfig mDvrConfig;
};
