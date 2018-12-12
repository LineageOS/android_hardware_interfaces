/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <android/hardware/thermal/2.0/IThermal.h>
#include <android/hardware/thermal/2.0/IThermalChangedCallback.h>
#include <android/hardware/thermal/2.0/types.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::android::sp;
using ::android::hardware::hidl_enum_range;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::thermal::V1_0::ThermalStatus;
using ::android::hardware::thermal::V1_0::ThermalStatusCode;
using ::android::hardware::thermal::V2_0::CoolingDevice;
using ::android::hardware::thermal::V2_0::CoolingType;
using ::android::hardware::thermal::V2_0::IThermal;
using ::android::hardware::thermal::V2_0::IThermalChangedCallback;
using ::android::hardware::thermal::V2_0::Temperature;
using ::android::hardware::thermal::V2_0::TemperatureThreshold;
using ::android::hardware::thermal::V2_0::TemperatureType;
using ::android::hardware::thermal::V2_0::ThrottlingSeverity;

constexpr char kCallbackNameNotifyThrottling[] = "notifyThrottling";
static const Temperature kThrottleTemp = {
    .type = TemperatureType::SKIN,
    .name = "test temperature sensor",
    .value = 98.6,
    .throttlingStatus = ThrottlingSeverity::CRITICAL,
};

class ThermalCallbackArgs {
   public:
    Temperature temperature;
};

// Callback class for receiving thermal event notifications from main class
class ThermalCallback : public ::testing::VtsHalHidlTargetCallbackBase<ThermalCallbackArgs>,
                        public IThermalChangedCallback {
   public:
    Return<void> notifyThrottling(const Temperature& temperature) override {
        ThermalCallbackArgs args;
        args.temperature = temperature;
        NotifyFromCallback(kCallbackNameNotifyThrottling, args);
        return Void();
    }
};

// Test environment for Thermal HIDL HAL.
class ThermalHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static ThermalHidlEnvironment* Instance() {
        static ThermalHidlEnvironment* instance = new ThermalHidlEnvironment;
        return instance;
    }

    void registerTestServices() override { registerTestService<IThermal>(); }

   private:
    ThermalHidlEnvironment() {}
};

// The main test class for THERMAL HIDL HAL 2.0.
class ThermalHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        mThermal = ::testing::VtsHalHidlTargetTestBase::getService<IThermal>(
            ThermalHidlEnvironment::Instance()->getServiceName<IThermal>());
        ASSERT_NE(mThermal, nullptr);
        mThermalCallback = new (std::nothrow) ThermalCallback();
        ASSERT_NE(mThermalCallback, nullptr);
        auto ret = mThermal->registerThermalChangedCallback(
            mThermalCallback, false, TemperatureType::SKIN,
            [](ThermalStatus status) { EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code); });
        ASSERT_TRUE(ret.isOk());
        // Expect to fail if register again
        ret = mThermal->registerThermalChangedCallback(
            mThermalCallback, false, TemperatureType::SKIN,
            [](ThermalStatus status) { EXPECT_NE(ThermalStatusCode::SUCCESS, status.code); });
        ASSERT_TRUE(ret.isOk());
    }

    virtual void TearDown() override {
        auto ret = mThermal->unregisterThermalChangedCallback(
            mThermalCallback,
            [](ThermalStatus status) { EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code); });
        ASSERT_TRUE(ret.isOk());
        // Expect to fail if unregister again
        ret = mThermal->unregisterThermalChangedCallback(
            mThermalCallback,
            [](ThermalStatus status) { EXPECT_NE(ThermalStatusCode::SUCCESS, status.code); });
        ASSERT_TRUE(ret.isOk());
    }

   protected:
    sp<IThermal> mThermal;
    sp<ThermalCallback> mThermalCallback;
};  // class ThermalHidlTest

// Test ThermalChangedCallback::notifyThrottling().
// This just calls into and back from our local ThermalChangedCallback impl.
// Note: a real thermal throttling event from the Thermal HAL could be
// inadvertently received here.
TEST_F(ThermalHidlTest, NotifyThrottlingTest) {
    auto ret = mThermalCallback->notifyThrottling(kThrottleTemp);
    ASSERT_TRUE(ret.isOk());
    auto res = mThermalCallback->WaitForCallback(kCallbackNameNotifyThrottling);
    EXPECT_TRUE(res.no_timeout);
    ASSERT_TRUE(res.args);
    EXPECT_EQ(kThrottleTemp, res.args->temperature);
}

// Test Thermal->registerThermalChangedCallback.
TEST_F(ThermalHidlTest, RegisterThermalChangedCallbackTest) {
    // Expect to fail with same callback
    auto ret = mThermal->registerThermalChangedCallback(
        mThermalCallback, false, TemperatureType::SKIN,
        [](ThermalStatus status) { EXPECT_NE(ThermalStatusCode::SUCCESS, status.code); });
    ASSERT_TRUE(ret.isOk());
    sp<ThermalCallback> localThermalCallback = new (std::nothrow) ThermalCallback();
    // Expect to succeed with different callback
    ret = mThermal->registerThermalChangedCallback(
        localThermalCallback, false, TemperatureType::SKIN,
        [](ThermalStatus status) { EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code); });
    ASSERT_TRUE(ret.isOk());
    // Remove the local callback.
    ret = mThermal->unregisterThermalChangedCallback(
        localThermalCallback,
        [](ThermalStatus status) { EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code); });
    ASSERT_TRUE(ret.isOk());
}

// Test Thermal->unregisterThermalChangedCallback.
TEST_F(ThermalHidlTest, UnregisterThermalChangedCallbackTest) {
    sp<ThermalCallback> localThermalCallback = new (std::nothrow) ThermalCallback();
    // Expect to fail as the callback was not registered before
    auto ret = mThermal->unregisterThermalChangedCallback(
        localThermalCallback,
        [](ThermalStatus status) { EXPECT_NE(ThermalStatusCode::SUCCESS, status.code); });
    ASSERT_TRUE(ret.isOk());
    // Register a local callback
    ret = mThermal->registerThermalChangedCallback(
        localThermalCallback, false, TemperatureType::SKIN,
        [](ThermalStatus status) { EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code); });
    ASSERT_TRUE(ret.isOk());
    // Expect to succeed with callback removed
    ret = mThermal->unregisterThermalChangedCallback(
        localThermalCallback,
        [](ThermalStatus status) { EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code); });
    ASSERT_TRUE(ret.isOk());
    // Expect to fail as the callback has been unregistered already
    ret = mThermal->unregisterThermalChangedCallback(
        localThermalCallback,
        [](ThermalStatus status) { EXPECT_NE(ThermalStatusCode::SUCCESS, status.code); });
    ASSERT_TRUE(ret.isOk());
}

// Sanity test for Thermal::getCurrentTemperatures().
TEST_F(ThermalHidlTest, TemperatureTest) {
    mThermal->getCurrentTemperatures(false, TemperatureType::SKIN,
                                     [](ThermalStatus status, hidl_vec<Temperature> temperatures) {
                                         if (temperatures.size()) {
                                             EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
                                         } else {
                                             EXPECT_NE(ThermalStatusCode::SUCCESS, status.code);
                                         }
                                     });
    auto types = hidl_enum_range<TemperatureType>();
    for (const auto& type : types) {
        mThermal->getCurrentTemperatures(
            true, type, [&type](ThermalStatus status, hidl_vec<Temperature> temperatures) {
                if (temperatures.size()) {
                    EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
                } else {
                    EXPECT_NE(ThermalStatusCode::SUCCESS, status.code);
                }
                for (int i = 0; i < temperatures.size(); ++i) {
                    EXPECT_EQ(type, temperatures[i].type);
                }
            });
    }
}

// Sanity test for Thermal::getTemperatureThresholds().
TEST_F(ThermalHidlTest, TemperatureThresholdTest) {
    mThermal->getTemperatureThresholds(
        false, TemperatureType::SKIN,
        [](ThermalStatus status, hidl_vec<TemperatureThreshold> temperatures) {
            if (temperatures.size()) {
                EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
            } else {
                EXPECT_NE(ThermalStatusCode::SUCCESS, status.code);
            }
        });
    for (int i = static_cast<int>(TemperatureType::UNKNOWN);
         i <= static_cast<int>(TemperatureType::POWER_AMPLIFIER); ++i) {
        auto type = static_cast<TemperatureType>(i);
        mThermal->getTemperatureThresholds(
            true, type, [&type](ThermalStatus status, hidl_vec<TemperatureThreshold> temperatures) {
                if (temperatures.size()) {
                    EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
                } else {
                    EXPECT_NE(ThermalStatusCode::SUCCESS, status.code);
                }
                for (int i = 0; i < temperatures.size(); ++i) {
                    EXPECT_EQ(type, temperatures[i].type);
                }
            });
    }
}

// Sanity test for Thermal::getCurrentCoolingDevices().
TEST_F(ThermalHidlTest, CoolingDeviceTest) {
    mThermal->getCurrentCoolingDevices(
        false, CoolingType::CPU, [](ThermalStatus status, hidl_vec<CoolingDevice> cooling_devices) {
            if (cooling_devices.size()) {
                EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
            } else {
                EXPECT_NE(ThermalStatusCode::SUCCESS, status.code);
            }
        });
    for (int i = 0; i <= static_cast<int>(CoolingType::COMPONENT); ++i) {
        auto type = static_cast<CoolingType>(i);
        mThermal->getCurrentCoolingDevices(
            true, type, [&type](ThermalStatus status, hidl_vec<CoolingDevice> cooling_devices) {
                if (cooling_devices.size()) {
                    EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
                } else {
                    EXPECT_NE(ThermalStatusCode::SUCCESS, status.code);
                }
                for (int i = 0; i < cooling_devices.size(); ++i) {
                    EXPECT_EQ(type, cooling_devices[i].type);
                }
            });
    }
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(ThermalHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    ThermalHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    cout << "Test result = " << status << std::endl;
    return status;
}
