/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "Tuner_hidl_hal_test"

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/tv/tuner/1.0/IDemux.h>
#include <android/hardware/tv/tuner/1.0/IDemuxCallback.h>
#include <android/hardware/tv/tuner/1.0/IDescrambler.h>
#include <android/hardware/tv/tuner/1.0/IFrontend.h>
#include <android/hardware/tv/tuner/1.0/IFrontendCallback.h>
#include <android/hardware/tv/tuner/1.0/ITuner.h>
#include <android/hardware/tv/tuner/1.0/types.h>
#include <binder/MemoryDealer.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/Status.h>
#include <hidlmemory/FrameworkUtils.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>

#define WAIT_TIMEOUT 3000000000

using android::Condition;
using android::IMemory;
using android::IMemoryHeap;
using android::MemoryDealer;
using android::Mutex;
using android::sp;
using android::hardware::fromHeap;
using android::hardware::hidl_string;
using android::hardware::hidl_vec;
using android::hardware::HidlMemory;
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::tv::tuner::V1_0::FrontendAtscModulation;
using android::hardware::tv::tuner::V1_0::FrontendAtscSettings;
using android::hardware::tv::tuner::V1_0::FrontendDvbtSettings;
using android::hardware::tv::tuner::V1_0::FrontendEventType;
using android::hardware::tv::tuner::V1_0::FrontendId;
using android::hardware::tv::tuner::V1_0::FrontendInnerFec;
using android::hardware::tv::tuner::V1_0::FrontendSettings;
using android::hardware::tv::tuner::V1_0::IDemux;
using android::hardware::tv::tuner::V1_0::IDemuxCallback;
using android::hardware::tv::tuner::V1_0::IDescrambler;
using android::hardware::tv::tuner::V1_0::IFrontend;
using android::hardware::tv::tuner::V1_0::IFrontendCallback;
using android::hardware::tv::tuner::V1_0::ITuner;
using android::hardware::tv::tuner::V1_0::Result;

namespace {

class FrontendCallback : public IFrontendCallback {
  public:
    virtual Return<void> onEvent(FrontendEventType frontendEventType) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        mEventReceived = true;
        mEventType = frontendEventType;
        mMsgCondition.signal();
        return Void();
    }

    virtual Return<void> onDiseqcMessage(const hidl_vec<uint8_t>& diseqcMessage) override {
        android::Mutex::Autolock autoLock(mMsgLock);
        mDiseqcMessageReceived = true;
        mEventMessage = diseqcMessage;
        mMsgCondition.signal();
        return Void();
    }

    void testOnEvent(sp<IFrontend>& frontend, FrontendSettings settings);
    void testOnDiseqcMessage(sp<IFrontend>& frontend, FrontendSettings settings);

  private:
    bool mEventReceived = false;
    bool mDiseqcMessageReceived = false;
    FrontendEventType mEventType;
    hidl_vec<uint8_t> mEventMessage;
    android::Mutex mMsgLock;
    android::Condition mMsgCondition;
};

void FrontendCallback::testOnEvent(sp<IFrontend>& frontend, FrontendSettings settings) {
    Result result = frontend->tune(settings);

    EXPECT_TRUE(result == Result::SUCCESS);

    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mEventReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "event not received within timeout";
            return;
        }
    }
}

void FrontendCallback::testOnDiseqcMessage(sp<IFrontend>& frontend, FrontendSettings settings) {
    Result result = frontend->tune(settings);

    EXPECT_TRUE(result == Result::SUCCESS);

    android::Mutex::Autolock autoLock(mMsgLock);
    while (!mDiseqcMessageReceived) {
        if (-ETIMEDOUT == mMsgCondition.waitRelative(mMsgLock, WAIT_TIMEOUT)) {
            EXPECT_TRUE(false) << "diseqc message not received within timeout";
            return;
        }
    }
}

// Test environment for Tuner HIDL HAL.
class TunerHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
  public:
    // get the test environment singleton
    static TunerHidlEnvironment* Instance() {
        static TunerHidlEnvironment* instance = new TunerHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<ITuner>(); }
};

class TunerHidlTest : public ::testing::VtsHalHidlTargetTestBase {
  public:
    virtual void SetUp() override {
        mService = ::testing::VtsHalHidlTargetTestBase::getService<ITuner>(
                TunerHidlEnvironment::Instance()->getServiceName<ITuner>());
        ASSERT_NE(mService, nullptr);
    }

    sp<ITuner> mService;

  protected:
    static void description(const std::string& description) {
        RecordProperty("description", description);
    }

    sp<IFrontend> mFrontend;
    sp<FrontendCallback> mFrontendCallback;
    sp<IDescrambler> mDescrambler;
    sp<IDemux> mDemux;
    uint32_t mDemuxId;

    ::testing::AssertionResult createFrontend(int32_t frontendId);
    ::testing::AssertionResult tuneFrontend(int32_t frontendId);
    ::testing::AssertionResult stopTuneFrontend(int32_t frontendId);
    ::testing::AssertionResult closeFrontend(int32_t frontendId);
    ::testing::AssertionResult createDemux();
    ::testing::AssertionResult createDemuxWithFrontend(int32_t frontendId);
    ::testing::AssertionResult closeDemux();
    ::testing::AssertionResult createDescrambler();
    ::testing::AssertionResult closeDescrambler();
};

::testing::AssertionResult TunerHidlTest::createFrontend(int32_t frontendId) {
    Result status;

    mService->openFrontendById(frontendId, [&](Result result, const sp<IFrontend>& frontend) {
        mFrontend = frontend;
        status = result;
    });
    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    mFrontendCallback = new FrontendCallback();
    auto callbackStatus = mFrontend->setCallback(mFrontendCallback);

    return ::testing::AssertionResult(callbackStatus.isOk());
}

::testing::AssertionResult TunerHidlTest::tuneFrontend(int32_t frontendId) {
    if (createFrontend(frontendId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    // Frontend Settings for testing
    FrontendSettings frontendSettings;
    FrontendAtscSettings frontendAtscSettings{
            .frequency = 0,
            .modulation = FrontendAtscModulation::UNDEFINED,
    };
    frontendSettings.atsc() = frontendAtscSettings;
    mFrontendCallback->testOnEvent(mFrontend, frontendSettings);

    FrontendDvbtSettings frontendDvbtSettings{
            .frequency = 0,
            .modulation = FrontendAtscModulation::UNDEFINED,
            .fec = FrontendInnerFec::FEC_UNDEFINED,
    };
    frontendSettings.dvbt(frontendDvbtSettings);
    mFrontendCallback->testOnEvent(mFrontend, frontendSettings);

    return ::testing::AssertionResult(true);
}

::testing::AssertionResult TunerHidlTest::stopTuneFrontend(int32_t frontendId) {
    Result status;
    if (createFrontend(frontendId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mFrontend->stopTune();
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::closeFrontend(int32_t frontendId) {
    Result status;
    if (createFrontend(frontendId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mFrontend->close();
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::createDemux() {
    Result status;

    mService->openDemux([&](Result result, uint32_t demuxId, const sp<IDemux>& demux) {
        mDemux = demux;
        mDemuxId = demuxId;
        status = result;
    });
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::createDemuxWithFrontend(int32_t frontendId) {
    Result status;

    if (createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    if (createFrontend(frontendId) == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mDemux->setFrontendDataSource(frontendId);

    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::closeDemux() {
    Result status;
    if (createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mDemux->close();
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

::testing::AssertionResult TunerHidlTest::createDescrambler() {
    Result status;

    mService->openDescrambler([&](Result result, const sp<IDescrambler>& descrambler) {
        mDescrambler = descrambler;
        status = result;
    });
    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    if (createDemux() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mDescrambler->setDemuxSource(mDemuxId);
    if (status != Result::SUCCESS) {
        return ::testing::AssertionFailure();
    }

    // Test if demux source can be set more than once.
    status = mDescrambler->setDemuxSource(mDemuxId);
    return ::testing::AssertionResult(status == Result::INVALID_STATE);
}

::testing::AssertionResult TunerHidlTest::closeDescrambler() {
    Result status;
    if (createDescrambler() == ::testing::AssertionFailure()) {
        return ::testing::AssertionFailure();
    }

    status = mDescrambler->close();
    return ::testing::AssertionResult(status == Result::SUCCESS);
}

TEST_F(TunerHidlTest, CreateFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("Create Frontends");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(createFrontend(feIds[i]));
    }
}

TEST_F(TunerHidlTest, TuneFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("Tune Frontends and check callback onEvent");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(tuneFrontend(feIds[i]));
    }
}

TEST_F(TunerHidlTest, StopTuneFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("stopTune Frontends");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(stopTuneFrontend(feIds[i]));
    }
}

TEST_F(TunerHidlTest, CloseFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("Close Frontends");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(closeFrontend(feIds[i]));
    }
}

TEST_F(TunerHidlTest, CreateDemux) {
    description("Create Demux");

    ASSERT_TRUE(createDemux());
}

TEST_F(TunerHidlTest, CreateDemuxWithFrontend) {
    Result status;
    hidl_vec<FrontendId> feIds;

    description("Create Demux with Frontend");
    mService->getFrontendIds([&](Result result, const hidl_vec<FrontendId>& frontendIds) {
        status = result;
        feIds = frontendIds;
    });

    if (feIds.size() == 0) {
        ALOGW("[   WARN   ] Frontend isn't available");
        return;
    }

    for (size_t i = 0; i < feIds.size(); i++) {
        ASSERT_TRUE(createDemuxWithFrontend(feIds[i]));
    }
}

TEST_F(TunerHidlTest, CloseDemux) {
    description("Close Demux");

    ASSERT_TRUE(closeDemux());
}

TEST_F(TunerHidlTest, CreateDescrambler) {
    description("Create Descrambler");

    ASSERT_TRUE(createDescrambler());
}

TEST_F(TunerHidlTest, CloseDescrambler) {
    description("Close Descrambler");

    ASSERT_TRUE(closeDescrambler());
}

}  // namespace

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(TunerHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    TunerHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
