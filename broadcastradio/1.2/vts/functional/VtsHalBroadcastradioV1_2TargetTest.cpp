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
#include <android/hardware/broadcastradio/1.1/IBroadcastRadio.h>
#include <android/hardware/broadcastradio/1.2/IBroadcastRadioFactory.h>
#include <android/hardware/broadcastradio/1.2/ITuner.h>
#include <android/hardware/broadcastradio/1.2/ITunerCallback.h>
#include <android/hardware/broadcastradio/1.2/types.h>
#include <broadcastradio-vts-utils/call-barrier.h>
#include <broadcastradio-vts-utils/mock-timeout.h>
#include <broadcastradio-vts-utils/pointer-utils.h>
#include <cutils/native_handle.h>
#include <cutils/properties.h>
#include <gmock/gmock.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/threads.h>

#include <chrono>

namespace android {
namespace hardware {
namespace broadcastradio {
namespace V1_2 {
namespace vts {

using namespace std::chrono_literals;

using testing::_;
using testing::AnyNumber;
using testing::ByMove;
using testing::DoAll;
using testing::Invoke;
using testing::SaveArg;

using broadcastradio::vts::CallBarrier;
using V1_0::BandConfig;
using V1_0::Class;
using V1_0::MetaData;
using V1_0::MetadataKey;
using V1_0::MetadataType;
using V1_1::IBroadcastRadio;
using V1_1::ProgramInfo;
using V1_1::ProgramListResult;
using V1_1::ProgramSelector;
using V1_1::Properties;

using broadcastradio::vts::clearAndWait;

static constexpr auto kConfigTimeout = 10s;
static constexpr auto kConnectModuleTimeout = 1s;

static void printSkipped(std::string msg) {
    std::cout << "[  SKIPPED ] " << msg << std::endl;
}

struct TunerCallbackMock : public ITunerCallback {
    TunerCallbackMock() { EXPECT_CALL(*this, hardwareFailure()).Times(0); }

    MOCK_METHOD0(hardwareFailure, Return<void>());
    MOCK_TIMEOUT_METHOD2(configChange, Return<void>(Result, const BandConfig&));
    MOCK_METHOD2(tuneComplete, Return<void>(Result, const V1_0::ProgramInfo&));
    MOCK_TIMEOUT_METHOD2(tuneComplete_1_1, Return<void>(Result, const ProgramSelector&));
    MOCK_METHOD1(afSwitch, Return<void>(const V1_0::ProgramInfo&));
    MOCK_METHOD1(antennaStateChange, Return<void>(bool connected));
    MOCK_METHOD1(trafficAnnouncement, Return<void>(bool active));
    MOCK_METHOD1(emergencyAnnouncement, Return<void>(bool active));
    MOCK_METHOD3(newMetadata, Return<void>(uint32_t ch, uint32_t subCh, const hidl_vec<MetaData>&));
    MOCK_METHOD1(backgroundScanAvailable, Return<void>(bool));
    MOCK_TIMEOUT_METHOD1(backgroundScanComplete, Return<void>(ProgramListResult));
    MOCK_METHOD0(programListChanged, Return<void>());
    MOCK_TIMEOUT_METHOD1(currentProgramInfoChanged, Return<void>(const ProgramInfo&));
    MOCK_METHOD1(parametersUpdated, Return<void>(const hidl_vec<VendorKeyValue>& parameters));
};

class BroadcastRadioHalTest : public ::testing::VtsHalHidlTargetTestBase,
                              public ::testing::WithParamInterface<Class> {
   protected:
    virtual void SetUp() override;
    virtual void TearDown() override;

    bool openTuner();

    Class radioClass;
    bool skipped = false;

    sp<IBroadcastRadio> mRadioModule;
    sp<ITuner> mTuner;
    sp<TunerCallbackMock> mCallback = new TunerCallbackMock();

   private:
    const BandConfig& getBand(unsigned idx);

    hidl_vec<BandConfig> mBands;
};

void BroadcastRadioHalTest::SetUp() {
    radioClass = GetParam();

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
    EXPECT_GT(prop11.supportedProgramTypes.size(), 0u);
    EXPECT_GT(prop11.supportedIdentifierTypes.size(), 0u);
    if (radioClass == Class::AM_FM) {
        EXPECT_GT(prop10.bands.size(), 0u);
    }
    mBands = prop10.bands;
}

void BroadcastRadioHalTest::TearDown() {
    mTuner.clear();
    mRadioModule.clear();
    clearAndWait(mCallback, 1s);
}

bool BroadcastRadioHalTest::openTuner() {
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
    auto hidlResult = mRadioModule->openTuner(getBand(0), true, mCallback, openCb);

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
 * Test IBroadcastRadio::get|setParameters() methods called with no parameters.
 *
 * Verifies that:
 *  - callback is called for empty parameters set.
 */
TEST_P(BroadcastRadioHalTest, NoParameters) {
    if (skipped) return;

    ASSERT_TRUE(openTuner());

    hidl_vec<VendorKeyValue> halResults = {};
    bool wasCalled = false;
    auto cb = [&](hidl_vec<VendorKeyValue> results) {
        wasCalled = true;
        halResults = results;
    };

    auto hidlResult = mTuner->setParameters({}, cb);
    ASSERT_TRUE(hidlResult.isOk());
    ASSERT_TRUE(wasCalled);
    ASSERT_EQ(0u, halResults.size());

    wasCalled = false;
    hidlResult = mTuner->getParameters({}, cb);
    ASSERT_TRUE(hidlResult.isOk());
    ASSERT_TRUE(wasCalled);
    ASSERT_EQ(0u, halResults.size());
}

/**
 * Test IBroadcastRadio::get|setParameters() methods called with unknown parameters.
 *
 * Verifies that:
 *  - unknown parameters are ignored;
 *  - callback is called also for empty results set.
 */
TEST_P(BroadcastRadioHalTest, UnknownParameters) {
    if (skipped) return;

    ASSERT_TRUE(openTuner());

    hidl_vec<VendorKeyValue> halResults = {};
    bool wasCalled = false;
    auto cb = [&](hidl_vec<VendorKeyValue> results) {
        wasCalled = true;
        halResults = results;
    };

    auto hidlResult = mTuner->setParameters({{"com.google.unknown", "dummy"}}, cb);
    ASSERT_TRUE(hidlResult.isOk());
    ASSERT_TRUE(wasCalled);
    ASSERT_EQ(0u, halResults.size());

    wasCalled = false;
    hidlResult = mTuner->getParameters({{"com.google.unknown*", "dummy"}}, cb);
    ASSERT_TRUE(hidlResult.isOk());
    ASSERT_TRUE(wasCalled);
    ASSERT_EQ(0u, halResults.size());
}

// TODO(b/69860743): implement VerifyIdentifiersFormat test when
// the new program list fetching mechanism is implemented

INSTANTIATE_TEST_CASE_P(BroadcastRadioHalTestCases, BroadcastRadioHalTest,
                        ::testing::Values(Class::AM_FM, Class::SAT, Class::DT));

}  // namespace vts
}  // namespace V1_2
}  // namespace broadcastradio
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
