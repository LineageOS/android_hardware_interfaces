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

#define LOG_TAG "BroadcastRadioHidlHalTest"
#include <VtsHalHidlTargetTestBase.h>
#include <android-base/logging.h>
#include <cutils/native_handle.h>
#include <cutils/properties.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/threads.h>

#include <android/hardware/broadcastradio/1.1/IBroadcastRadioFactory.h>
#include <android/hardware/broadcastradio/1.0/IBroadcastRadio.h>
#include <android/hardware/broadcastradio/1.1/ITuner.h>
#include <android/hardware/broadcastradio/1.1/ITunerCallback.h>
#include <android/hardware/broadcastradio/1.1/types.h>


namespace V1_0 = ::android::hardware::broadcastradio::V1_0;

using ::android::sp;
using ::android::Mutex;
using ::android::Condition;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::broadcastradio::V1_0::BandConfig;
using ::android::hardware::broadcastradio::V1_0::Class;
using ::android::hardware::broadcastradio::V1_0::Direction;
using ::android::hardware::broadcastradio::V1_0::IBroadcastRadio;
using ::android::hardware::broadcastradio::V1_0::MetaData;
using ::android::hardware::broadcastradio::V1_0::Properties;
using ::android::hardware::broadcastradio::V1_1::IBroadcastRadioFactory;
using ::android::hardware::broadcastradio::V1_1::ITuner;
using ::android::hardware::broadcastradio::V1_1::ITunerCallback;
using ::android::hardware::broadcastradio::V1_1::ProgramInfo;
using ::android::hardware::broadcastradio::V1_1::Result;
using ::android::hardware::broadcastradio::V1_1::ProgramListResult;


// The main test class for Broadcast Radio HIDL HAL.

class BroadcastRadioHidlTest : public ::testing::VtsHalHidlTargetTestBase {
 protected:
    virtual void SetUp() override {
        auto factory = ::testing::VtsHalHidlTargetTestBase::getService<IBroadcastRadioFactory>();
        if (factory != 0) {
            factory->connectModule(Class::AM_FM,
                             [&](Result retval, const ::android::sp<IBroadcastRadio>& result) {
                if (retval == Result::OK) {
                  mRadio = IBroadcastRadio::castFrom(result);
                }
            });
        }
        mTunerCallback = new MyCallback(this);
        ASSERT_NE(nullptr, mRadio.get());
        ASSERT_NE(nullptr, mTunerCallback.get());
    }

    virtual void TearDown() override {
        mTuner.clear();
        mRadio.clear();
    }

    class MyCallback : public ITunerCallback {
     public:

        // ITunerCallback methods (see doc in ITunerCallback.hal)
        virtual Return<void> hardwareFailure() {
            ALOGI("%s", __FUNCTION__);
            mParentTest->onHwFailureCallback();
            return Void();
        }

        virtual Return<void> configChange(Result result, const BandConfig& config __unused) {
            ALOGI("%s result %d", __FUNCTION__, result);
            mParentTest->onResultCallback(result);
            return Void();
        }

        virtual Return<void> tuneComplete(Result result __unused, const V1_0::ProgramInfo& info __unused) {
            return Void();
        }

        virtual Return<void> tuneComplete_1_1(Result result, const ProgramInfo& info __unused) {
            ALOGI("%s result %d", __FUNCTION__, result);
            mParentTest->onResultCallback(result);
            return Void();
        }

        virtual Return<void> afSwitch(const V1_0::ProgramInfo& info __unused) {
            return Void();
        }

        virtual Return<void> afSwitch_1_1(const ProgramInfo& info __unused) {
            return Void();
        }

        virtual Return<void> antennaStateChange(bool connected) {
            ALOGI("%s connected %d", __FUNCTION__, connected);
            return Void();
        }

        virtual Return<void> trafficAnnouncement(bool active) {
            ALOGI("%s active %d", __FUNCTION__, active);
            return Void();
        }

        virtual Return<void> emergencyAnnouncement(bool active) {
            ALOGI("%s active %d", __FUNCTION__, active);
            return Void();
        }

        virtual Return<void> newMetadata(uint32_t channel __unused, uint32_t subChannel __unused,
                           const ::android::hardware::hidl_vec<MetaData>& metadata __unused) {
            ALOGI("%s", __FUNCTION__);
            return Void();
        }

        virtual Return<void> backgroundScanAvailable(bool isAvailable __unused) {
            return Void();
        }

        virtual Return<void> backgroundScanComplete(ProgramListResult result __unused) {
            return Void();
        }

        virtual Return<void> programListChanged() {
            return Void();
        }

                MyCallback(BroadcastRadioHidlTest *parentTest) : mParentTest(parentTest) {}

     private:
        // BroadcastRadioHidlTest instance to which callbacks will be notified.
        BroadcastRadioHidlTest *mParentTest;
    };


    /**
     * Method called by MyCallback when a callback with no status or boolean value is received
     */
    void onCallback() {
        Mutex::Autolock _l(mLock);
        onCallback_l();
    }

    /**
     * Method called by MyCallback when hardwareFailure() callback is received
     */
    void onHwFailureCallback() {
        Mutex::Autolock _l(mLock);
        mHwFailure = true;
        onCallback_l();
    }

    /**
     * Method called by MyCallback when a callback with status is received
     */
    void onResultCallback(Result result) {
        Mutex::Autolock _l(mLock);
        mResultCallbackData = result;
        onCallback_l();
    }

    /**
     * Method called by MyCallback when a boolean indication is received
     */
    void onBoolCallback(bool result) {
        Mutex::Autolock _l(mLock);
        mBoolCallbackData = result;
        onCallback_l();
    }


        BroadcastRadioHidlTest() :
            mCallbackCalled(false), mBoolCallbackData(false),
            mResultCallbackData(Result::OK), mHwFailure(false) {}

    void onCallback_l() {
        if (!mCallbackCalled) {
            mCallbackCalled = true;
            mCallbackCond.broadcast();
        }
    }


    bool waitForCallback(nsecs_t reltime = 0) {
        Mutex::Autolock _l(mLock);
        nsecs_t endTime = systemTime() + reltime;
        while (!mCallbackCalled) {
            if (reltime == 0) {
                mCallbackCond.wait(mLock);
            } else {
                nsecs_t now = systemTime();
                if (now > endTime) {
                    return false;
                }
                mCallbackCond.waitRelative(mLock, endTime - now);
            }
        }
        return true;
    }

    bool getProperties();
    bool openTuner();
    bool checkAntenna();

    static const nsecs_t kConfigCallbacktimeoutNs = seconds_to_nanoseconds(10);
    static const nsecs_t kTuneCallbacktimeoutNs = seconds_to_nanoseconds(30);

    sp<IBroadcastRadio> mRadio;
    Properties mHalProperties;
    sp<ITuner> mTuner;
    sp<MyCallback> mTunerCallback;
    Mutex mLock;
    Condition mCallbackCond;
    bool mCallbackCalled;
    bool mBoolCallbackData;
    Result mResultCallbackData;
    bool mHwFailure;
};

// A class for test environment setup (kept since this file is a template).
class BroadcastRadioHidlEnvironment : public ::testing::Environment {
 public:
    virtual void SetUp() {}
    virtual void TearDown() {}
};

bool BroadcastRadioHidlTest::getProperties()
{
    if (mHalProperties.bands.size() == 0) {
        Result halResult = Result::NOT_INITIALIZED;
        Return<void> hidlReturn =
                mRadio->getProperties([&](Result result, const Properties& properties) {
                        halResult = result;
                        if (result == Result::OK) {
                            mHalProperties = properties;
                        }
                    });

        EXPECT_TRUE(hidlReturn.isOk());
        EXPECT_EQ(Result::OK, halResult);
        EXPECT_EQ(Class::AM_FM, mHalProperties.classId);
        EXPECT_GT(mHalProperties.numTuners, 0u);
        EXPECT_GT(mHalProperties.bands.size(), 0u);
    }
    return mHalProperties.bands.size() > 0;
}

bool BroadcastRadioHidlTest::openTuner()
{
    if (!getProperties()) {
        return false;
    }
    if (mTuner.get() == nullptr) {
        Result halResult = Result::NOT_INITIALIZED;
        auto hidlReturn = mRadio->openTuner(mHalProperties.bands[0], true, mTunerCallback,
                [&](Result result, const sp<V1_0::ITuner>& tuner) {
                    halResult = result;
                    if (result == Result::OK) {
                        mTuner = ITuner::castFrom(tuner);
                    }
                });
        EXPECT_TRUE(hidlReturn.isOk());
        EXPECT_EQ(Result::OK, halResult);
        EXPECT_TRUE(waitForCallback(kConfigCallbacktimeoutNs));
    }
    EXPECT_NE(nullptr, mTuner.get());
    return nullptr != mTuner.get();
}

bool BroadcastRadioHidlTest::checkAntenna()
{
    BandConfig halConfig;
    Result halResult = Result::NOT_INITIALIZED;
    Return<void> hidlReturn =
            mTuner->getConfiguration([&](Result result, const BandConfig& config) {
                halResult = result;
                if (result == Result::OK) {
                    halConfig = config;
                }
            });

    return ((halResult == Result::OK) && (halConfig.antennaConnected == true));
}


/**
 * Test IBroadcastRadio::getProperties() method
 *
 * Verifies that:
 *  - the HAL implements the method
 *  - the method returns 0 (no error)
 *  - the implementation class is AM_FM
 *  - the implementation supports at least one tuner
 *  - the implementation supports at one band
 */
TEST_F(BroadcastRadioHidlTest, GetProperties) {
    EXPECT_TRUE(getProperties());
}

/**
 * Test IBroadcastRadio::openTuner() method
 *
 * Verifies that:
 *  - the HAL implements the method
 *  - the method returns 0 (no error) and a valid ITuner interface
 */
TEST_F(BroadcastRadioHidlTest, OpenTuner) {
    EXPECT_TRUE(openTuner());
}

/**
 * Test ITuner::setConfiguration() and getConfiguration methods
 *
 * Verifies that:
 *  - the HAL implements both methods
 *  - the methods return 0 (no error)
 *  - the configuration callback is received within kConfigCallbacktimeoutNs ns
 *  - the configuration read back from HAl has the same class Id
 */
TEST_F(BroadcastRadioHidlTest, SetAndGetConfiguration) {
    ASSERT_TRUE(openTuner());
    // test setConfiguration
    mCallbackCalled = false;
    Return<Result> hidlResult = mTuner->setConfiguration(mHalProperties.bands[0]);
    EXPECT_TRUE(hidlResult.isOk());
    EXPECT_EQ(Result::OK, hidlResult);
    EXPECT_TRUE(waitForCallback(kConfigCallbacktimeoutNs));
    EXPECT_EQ(Result::OK, mResultCallbackData);

    // test getConfiguration
    BandConfig halConfig;
    Result halResult;
    Return<void> hidlReturn =
            mTuner->getConfiguration([&](Result result, const BandConfig& config) {
                halResult = result;
                if (result == Result::OK) {
                    halConfig = config;
                }
            });
    EXPECT_TRUE(hidlReturn.isOk());
    EXPECT_EQ(Result::OK, halResult);
    EXPECT_EQ(mHalProperties.bands[0].type, halConfig.type);
}

/**
 * Test ITuner::scan
 *
 * Verifies that:
 *  - the HAL implements the method
 *  - the method returns 0 (no error)
 *  - the tuned callback is received within kTuneCallbacktimeoutNs ns
 */
TEST_F(BroadcastRadioHidlTest, Scan) {
    ASSERT_TRUE(openTuner());
    ASSERT_TRUE(checkAntenna());
    // test scan UP
    mCallbackCalled = false;
    Return<Result> hidlResult = mTuner->scan(Direction::UP, true);
    EXPECT_TRUE(hidlResult.isOk());
    EXPECT_EQ(Result::OK, hidlResult);
    EXPECT_TRUE(waitForCallback(kTuneCallbacktimeoutNs));

    // test scan DOWN
    mCallbackCalled = false;
    hidlResult = mTuner->scan(Direction::DOWN, true);
    EXPECT_TRUE(hidlResult.isOk());
    EXPECT_EQ(Result::OK, hidlResult);
    EXPECT_TRUE(waitForCallback(kTuneCallbacktimeoutNs));
}

/**
 * Test ITuner::step
 *
 * Verifies that:
 *  - the HAL implements the method
 *  - the method returns 0 (no error)
 *  - the tuned callback is received within kTuneCallbacktimeoutNs ns
 */
TEST_F(BroadcastRadioHidlTest, Step) {
    ASSERT_TRUE(openTuner());
    ASSERT_TRUE(checkAntenna());
    // test step UP
    mCallbackCalled = false;
    Return<Result> hidlResult = mTuner->step(Direction::UP, true);
    EXPECT_TRUE(hidlResult.isOk());
    EXPECT_EQ(Result::OK, hidlResult);
    EXPECT_TRUE(waitForCallback(kTuneCallbacktimeoutNs));

    // test step DOWN
    mCallbackCalled = false;
    hidlResult = mTuner->step(Direction::DOWN, true);
    EXPECT_TRUE(hidlResult.isOk());
    EXPECT_EQ(Result::OK, hidlResult);
    EXPECT_TRUE(waitForCallback(kTuneCallbacktimeoutNs));
}

/**
 * Test ITuner::tune,  getProgramInformation and cancel methods
 *
 * Verifies that:
 *  - the HAL implements the methods
 *  - the methods return 0 (no error)
 *  - the tuned callback is received within kTuneCallbacktimeoutNs ns after tune()
 */
TEST_F(BroadcastRadioHidlTest, TuneAndGetProgramInformationAndCancel) {
    ASSERT_TRUE(openTuner());
    ASSERT_TRUE(checkAntenna());

    // test tune
    ASSERT_GT(mHalProperties.bands[0].spacings.size(), 0u);
    ASSERT_GT(mHalProperties.bands[0].upperLimit, mHalProperties.bands[0].lowerLimit);

    // test scan UP
    uint32_t lowerLimit = mHalProperties.bands[0].lowerLimit;
    uint32_t upperLimit = mHalProperties.bands[0].upperLimit;
    uint32_t spacing = mHalProperties.bands[0].spacings[0];

    uint32_t channel =
            lowerLimit + (((upperLimit - lowerLimit) / 2 + spacing - 1) / spacing) * spacing;
    mCallbackCalled = false;
    mResultCallbackData = Result::NOT_INITIALIZED;
    Return<Result> hidlResult = mTuner->tune(channel, 0);
    EXPECT_TRUE(hidlResult.isOk());
    EXPECT_EQ(Result::OK, hidlResult);
    EXPECT_TRUE(waitForCallback(kTuneCallbacktimeoutNs));

    // test getProgramInformation
    ProgramInfo halInfo;
    Result halResult = Result::NOT_INITIALIZED;
    Return<void> hidlReturn = mTuner->getProgramInformation_1_1(
        [&](Result result, const ProgramInfo& info) {
            halResult = result;
            if (result == Result::OK) {
                halInfo = info;
            }
        });
    EXPECT_TRUE(hidlReturn.isOk());
    EXPECT_EQ(Result::OK, halResult);
    auto &halInfo_1_1 = halInfo.base;
    if (mResultCallbackData == Result::OK) {
        EXPECT_TRUE(halInfo_1_1.tuned);
        EXPECT_LE(halInfo_1_1.channel, upperLimit);
        EXPECT_GE(halInfo_1_1.channel, lowerLimit);
    } else {
        EXPECT_EQ(false, halInfo_1_1.tuned);
    }

    // test cancel
    mTuner->tune(lowerLimit, 0);
    hidlResult = mTuner->cancel();
    EXPECT_TRUE(hidlResult.isOk());
    EXPECT_EQ(Result::OK, hidlResult);
}


int main(int argc, char** argv) {
  ::testing::AddGlobalTestEnvironment(new BroadcastRadioHidlEnvironment);
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
