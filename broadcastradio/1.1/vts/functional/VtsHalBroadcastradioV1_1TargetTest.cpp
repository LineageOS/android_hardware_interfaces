/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "broadcastradio.vts"

#include <VtsHalHidlTargetTestBase.h>
#include <android-base/logging.h>
#include <call-barrier.h>
#include <cutils/native_handle.h>
#include <cutils/properties.h>
#include <gmock/gmock.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/threads.h>

#include <chrono>

#include <android/hardware/broadcastradio/1.1/IBroadcastRadio.h>
#include <android/hardware/broadcastradio/1.1/IBroadcastRadioFactory.h>
#include <android/hardware/broadcastradio/1.1/ITuner.h>
#include <android/hardware/broadcastradio/1.1/ITunerCallback.h>
#include <android/hardware/broadcastradio/1.1/types.h>

#include "mock-timeout.h"

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V1_1 {
namespace vts {

using namespace std::chrono_literals;

using testing::_;
using testing::AnyNumber;
using testing::ByMove;
using testing::DoAll;
using testing::SaveArg;

using broadcastradio::vts::CallBarrier;
using V1_0::BandConfig;
using V1_0::Class;
using V1_0::MetaData;

static constexpr auto kConfigTimeout = 10s;
static constexpr auto kConnectModuleTimeout = 1s;
static constexpr auto kTuneTimeout = 30s;
static constexpr auto kFullScanTimeout = 1min;

static void printSkipped(std::string msg) {
    std::cout << "[  SKIPPED ] " << msg << std::endl;
}

class TunerCallbackMock : public ITunerCallback {
   public:
    MOCK_METHOD0(hardwareFailure, Return<void>());
    MOCK_TIMEOUT_METHOD2(configChange, Return<void>(Result, const BandConfig&));
    MOCK_METHOD2(tuneComplete, Return<void>(Result, const V1_0::ProgramInfo&));
    MOCK_TIMEOUT_METHOD2(tuneComplete_1_1, Return<void>(Result, const ProgramInfo&));
    MOCK_METHOD1(afSwitch, Return<void>(const V1_0::ProgramInfo&));
    MOCK_METHOD1(afSwitch_1_1, Return<void>(const ProgramInfo&));
    MOCK_METHOD1(antennaStateChange, Return<void>(bool connected));
    MOCK_METHOD1(trafficAnnouncement, Return<void>(bool active));
    MOCK_METHOD1(emergencyAnnouncement, Return<void>(bool active));
    MOCK_METHOD3(newMetadata, Return<void>(uint32_t ch, uint32_t subCh, const hidl_vec<MetaData>&));
    MOCK_METHOD1(backgroundScanAvailable, Return<void>(bool));
    MOCK_TIMEOUT_METHOD1(backgroundScanComplete, Return<void>(ProgramListResult));
    MOCK_METHOD0(programListChanged, Return<void>());
};

class BroadcastRadioHalTest : public ::testing::VtsHalHidlTargetTestBase,
                              public ::testing::WithParamInterface<Class> {
   protected:
    virtual void SetUp() override;
    virtual void TearDown() override;

    // TODO(b/36864490): check all bands for good test conditions (ie. FM is more likely to have
    // any stations on the list, so don't pick AM blindly).
    bool openTuner(unsigned band);
    const BandConfig& getBand(unsigned idx);

    Class radioClass;
    bool skipped = false;

    sp<IBroadcastRadio> mRadioModule;
    sp<ITuner> mTuner;
    sp<TunerCallbackMock> mCallback = new TunerCallbackMock();

   private:
    hidl_vec<BandConfig> mBands;
};

void BroadcastRadioHalTest::SetUp() {
    radioClass = GetParam();

    // set general expectations for a callback
    EXPECT_CALL(*mCallback, hardwareFailure()).Times(0);

    // lookup HIDL service
    auto factory = getService<IBroadcastRadioFactory>();
    ASSERT_NE(nullptr, factory.get());

    // connect radio module
    Result connectResult;
    CallBarrier onConnect;
    factory->connectModule(radioClass, [&](Result ret, const sp<V1_0::IBroadcastRadio>& radio) {
        connectResult = ret;
        if (ret == Result::OK) mRadioModule = IBroadcastRadio::castFrom(radio);
        onConnect.call();
    });
    ASSERT_TRUE(onConnect.waitForCall(kConnectModuleTimeout));

    if (connectResult == Result::INVALID_ARGUMENTS) {
        printSkipped("This device class is not supported.");
        skipped = true;
        return;
    }
    ASSERT_EQ(connectResult, Result::OK);
    ASSERT_NE(nullptr, mRadioModule.get());

    // get module properties
    Properties prop11;
    auto& prop10 = prop11.base;
    auto propResult =
        mRadioModule->getProperties_1_1([&](const Properties& properties) { prop11 = properties; });

    ASSERT_TRUE(propResult.isOk());
    EXPECT_EQ(radioClass, prop10.classId);
    EXPECT_GT(prop10.numTuners, 0u);
    if (radioClass == Class::AM_FM) {
        EXPECT_GT(prop10.bands.size(), 0u);
    }
    mBands = prop10.bands;
}

void BroadcastRadioHalTest::TearDown() {
    mTuner.clear();
    mRadioModule.clear();
    // TODO(b/36864490): wait (with timeout) until mCallback has only one reference
}

bool BroadcastRadioHalTest::openTuner(unsigned band) {
    EXPECT_EQ(nullptr, mTuner.get());

    if (radioClass == Class::AM_FM) {
        EXPECT_TIMEOUT_CALL(*mCallback, configChange, Result::OK, _);
    }

    Result halResult = Result::NOT_INITIALIZED;
    auto openCb = [&](Result result, const sp<V1_0::ITuner>& tuner) {
        halResult = result;
        if (result != Result::OK) return;
        mTuner = ITuner::castFrom(tuner);
    };
    auto hidlResult = mRadioModule->openTuner(getBand(band), true, mCallback, openCb);

    EXPECT_TRUE(hidlResult.isOk());
    EXPECT_EQ(Result::OK, halResult);
    EXPECT_NE(nullptr, mTuner.get());
    if (radioClass == Class::AM_FM && mTuner != nullptr) {
        EXPECT_TIMEOUT_CALL_WAIT(*mCallback, configChange, kConfigTimeout);

        BandConfig halConfig;
        Result halResult = Result::NOT_INITIALIZED;
        mTuner->getConfiguration([&](Result result, const BandConfig& config) {
            halResult = result;
            halConfig = config;
        });
        EXPECT_EQ(Result::OK, halResult);
        EXPECT_TRUE(halConfig.antennaConnected);
    }

    EXPECT_NE(nullptr, mTuner.get());
    return nullptr != mTuner.get();
}

const BandConfig& BroadcastRadioHalTest::getBand(unsigned idx) {
    static const BandConfig dummyBandConfig = {};

    if (radioClass != Class::AM_FM) {
        ALOGD("Not AM/FM radio, returning dummy band config");
        return dummyBandConfig;
    }

    EXPECT_GT(mBands.size(), idx);
    if (mBands.size() <= idx) {
        ALOGD("Band index out of bound, returning dummy band config");
        return dummyBandConfig;
    }

    auto& band = mBands[idx];
    ALOGD("Returning %s band", toString(band.type).c_str());
    return band;
}

/**
 * Test IBroadcastRadio::openTuner() method called twice.
 *
 * Verifies that:
 *  - the openTuner method succeeds when called for the second time without
 *    deleting previous ITuner instance.
 *
 * This is a more strict requirement than in 1.0, where a second openTuner
 * might fail.
 */
TEST_P(BroadcastRadioHalTest, OpenTunerTwice) {
    if (skipped) return;
    ASSERT_TRUE(openTuner(0));

    Result halResult = Result::NOT_INITIALIZED;
    auto openCb = [&](Result result, const sp<V1_0::ITuner>&) { halResult = result; };
    auto hidlResult = mRadioModule->openTuner(getBand(0), true, mCallback, openCb);
    ASSERT_TRUE(hidlResult.isOk());
    ASSERT_EQ(Result::OK, halResult);
}

/**
 * Test tuning to program list entry.
 *
 * Verifies that:
 *  - getProgramList either succeeds or returns NOT_STARTED/NOT_READY status;
 *  - if the program list is NOT_STARTED, startBackgroundScan makes it completed
 *    within a full scan timeout and the next getProgramList call succeeds;
 *  - if the program list is not empty, tune_1_1 call succeeds.
 */
TEST_P(BroadcastRadioHalTest, TuneFromProgramList) {
    if (skipped) return;
    ASSERT_TRUE(openTuner(0));

    ProgramInfo firstProgram;
    bool isListEmpty;
    ProgramListResult getListResult = ProgramListResult::NOT_INITIALIZED;
    auto getListCb = [&](ProgramListResult result, const hidl_vec<ProgramInfo>& list) {
        ALOGD("getListCb(%s, ProgramInfo[%zu])", toString(result).c_str(), list.size());
        getListResult = result;
        if (result != ProgramListResult::OK) return;
        isListEmpty = (list.size() == 0);
        // don't copy the whole list out, it might be heavy
        if (!isListEmpty) firstProgram = list[0];
    };

    // first try...
    EXPECT_TIMEOUT_CALL(*mCallback, backgroundScanComplete, ProgramListResult::OK)
        .Times(AnyNumber());
    auto hidlResult = mTuner->getProgramList("", getListCb);
    ASSERT_TRUE(hidlResult.isOk());

    if (getListResult == ProgramListResult::NOT_STARTED) {
        auto result = mTuner->startBackgroundScan();
        ASSERT_EQ(ProgramListResult::OK, result);
        getListResult = ProgramListResult::NOT_READY;  // continue as in NOT_READY case
    }
    if (getListResult == ProgramListResult::NOT_READY) {
        EXPECT_TIMEOUT_CALL_WAIT(*mCallback, backgroundScanComplete, kFullScanTimeout);

        // second (last) try...
        hidlResult = mTuner->getProgramList("", getListCb);
        ASSERT_TRUE(hidlResult.isOk());
        ASSERT_EQ(ProgramListResult::OK, getListResult);
    }

    if (isListEmpty) {
        printSkipped("Program list is empty.");
        return;
    }

    ProgramInfo infoCb;
    EXPECT_CALL(*mCallback, tuneComplete(_, _));
    EXPECT_TIMEOUT_CALL(*mCallback, tuneComplete_1_1, Result::OK, _)
        .WillOnce(DoAll(SaveArg<1>(&infoCb), testing::Return(ByMove(Void()))));
    auto tuneResult = mTuner->tune_1_1(firstProgram.selector);
    ASSERT_EQ(Result::OK, tuneResult);
    EXPECT_TIMEOUT_CALL_WAIT(*mCallback, tuneComplete_1_1, kTuneTimeout);
    EXPECT_EQ(firstProgram.selector.primaryId, infoCb.selector.primaryId);
}

INSTANTIATE_TEST_CASE_P(BroadcastRadioHalTestCases, BroadcastRadioHalTest,
                        ::testing::Values(Class::AM_FM, Class::SAT, Class::DT));

}  // namespace vts
}  // namespace V1_1
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
