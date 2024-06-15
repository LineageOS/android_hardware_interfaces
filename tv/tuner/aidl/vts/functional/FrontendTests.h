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

#include <aidl/android/hardware/tv/tuner/BnFrontendCallback.h>
#include <aidl/android/hardware/tv/tuner/IFrontend.h>
#include <aidl/android/hardware/tv/tuner/ITuner.h>

#include <gtest/gtest.h>
#include <log/log.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>

#include "DvrTests.h"
#include "VtsHalTvTunerTestConfigurations.h"
#include "utils/IpStreamer.h"

#define WAIT_TIMEOUT 3000000000
#define INVALID_ID -1

using android::Condition;
using android::Mutex;

using ::testing::AssertionResult;

using namespace aidl::android::hardware::tv::tuner;
using namespace std;

#define INVALID_ID -1
#define WAIT_TIMEOUT 3000000000

class FrontendCallback : public BnFrontendCallback {
  public:
    virtual ndk::ScopedAStatus onEvent(FrontendEventType frontendEventType) override;
    virtual ndk::ScopedAStatus onScanMessage(FrontendScanMessageType type,
                                             const FrontendScanMessage& message) override;

    void tuneTestOnLock(std::shared_ptr<IFrontend>& frontend, FrontendSettings settings);
    void scanTest(std::shared_ptr<IFrontend>& frontend, FrontendConfig config,
                  FrontendScanType type);

    // Helper methods
    int64_t getTargetFrequency(FrontendSettings& settings);
    void resetBlindScanStartingFrequency(FrontendConfig& config, int64_t resetingFreq);

  private:
    void readFrontendScanMessage_Modulation(FrontendModulation modulation);

    bool mEventReceived = false;
    bool mScanMessageReceived = false;
    bool mLockMsgReceived = false;
    bool mScanMsgProcessed = true;
    FrontendScanMessageType mScanMessageType;
    FrontendScanMessage mScanMessage;
    vector<int8_t> mEventMessage;
    android::Mutex mMsgLock;
    android::Condition mMsgCondition;
    android::Condition mLockMsgCondition;
};

class FrontendTests {
  public:
    void setService(std::shared_ptr<ITuner> tuner) {
        mService = tuner;
        getDvrTests()->setService(tuner);
        getDefaultSoftwareFrontendPlaybackConfig(mDvrConfig);
    }

    AssertionResult getFrontendIds();
    AssertionResult getFrontendInfo(int32_t frontendId);
    AssertionResult openFrontendById(int32_t frontendId);
    AssertionResult setFrontendCallback();
    AssertionResult scanFrontend(FrontendConfig config, FrontendScanType type);
    AssertionResult stopScanFrontend();
    AssertionResult setLnb(int32_t lnbId);
    AssertionResult tuneFrontend(FrontendConfig config, bool testWithDemux);
    void verifyFrontendStatus(vector<FrontendStatusType> statusTypes,
                              vector<FrontendStatus> expectStatuses);
    AssertionResult stopTuneFrontend(bool testWithDemux);
    AssertionResult closeFrontend();

    AssertionResult linkCiCam(int32_t ciCamId);
    AssertionResult unlinkCiCam(int32_t ciCamId);
    AssertionResult verifyHardwareInfo();
    AssertionResult removeOutputPid(int32_t removePid);

    void getFrontendIdByType(FrontendType feType, int32_t& feId);
    void tuneTest(FrontendConfig frontendConf);
    void scanTest(FrontendConfig frontend, FrontendScanType type);
    void debugInfoTest(FrontendConfig frontendConf);
    void maxNumberOfFrontendsTest();
    void statusReadinessTest(FrontendConfig frontendConf);

    void setDvrTests(DvrTests* dvrTests) { mExternalDvrTests = dvrTests; }
    void setDemux(std::shared_ptr<IDemux> demux) { getDvrTests()->setDemux(demux); }
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
                .packetSize = static_cast<int64_t>(188),
        };
        dvrConfig.type = DvrType::PLAYBACK;
        dvrConfig.playbackInputFile = "/data/local/tmp/test.es";
        dvrConfig.bufferSize = FMQ_SIZE_4M;
        dvrConfig.settings.set<DvrSettings::playback>(playbackSettings);
    }

    DvrTests* getDvrTests() {
        return (mExternalDvrTests != nullptr ? mExternalDvrTests : &mDvrTests);
    }

    std::shared_ptr<ITuner> mService;
    std::shared_ptr<IFrontend> mFrontend;
    FrontendInfo mFrontendInfo;
    std::shared_ptr<FrontendCallback> mFrontendCallback;
    vector<int32_t> mFeIds;

    DvrTests mDvrTests;
    DvrTests* mExternalDvrTests = nullptr;
    bool mIsSoftwareFe = false;
    DvrConfig mDvrConfig;
};
