/*
 * Copyright (C) 2023 The Android Open Source Project
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

#define LOG_TAG "thermal_hidl_wrapper_test"
#include <VtsHalHidlTargetCallbackBase.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/thermal/BnThermal.h>
#include <aidl/android/hardware/thermal/BnThermalChangedCallback.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_ibinder.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/binder_status.h>
#include <android/hardware/thermal/2.0/IThermal.h>
#include <android/hardware/thermal/2.0/IThermalChangedCallback.h>
#include <android/hardware/thermal/2.0/types.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <thermalutils/ThermalHidlWrapper.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace aidl::android::hardware::thermal {

namespace {

using ::android::sp;
using ::android::hardware::hidl_enum_range;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

using ::android::hardware::thermal::V1_0::ThermalStatus;
using ::android::hardware::thermal::V1_0::ThermalStatusCode;
using ::android::hardware::thermal::V2_0::CoolingDevice;
using ::android::hardware::thermal::V2_0::CoolingType;
using IThermal_2_0 = ::android::hardware::thermal::V2_0::IThermal;
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

// The main test class for THERMAL HIDL HAL 2.0.
class ThermalHidlWrapperTest : public ::testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        AIBinder* binder = AServiceManager_waitForService(GetParam().c_str());
        ASSERT_NE(binder, nullptr);
        mThermal = sp<ThermalHidlWrapper>::make(IThermal::fromBinder(ndk::SpAIBinder(binder)));
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

    void TearDown() override {
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
    sp<IThermal_2_0> mThermal;
    sp<ThermalCallback> mThermalCallback;
};  // class ThermalHidlWrapperTest

// Test ThermalChangedCallback::notifyThrottling().
// This just calls into and back from our local ThermalChangedCallback impl.
TEST_P(ThermalHidlWrapperTest, NotifyThrottlingTest) {
    sp<ThermalCallback> thermalCallback = new (std::nothrow) ThermalCallback();
    auto ret = thermalCallback->notifyThrottling(kThrottleTemp);
    ASSERT_TRUE(ret.isOk());
    auto res = thermalCallback->WaitForCallback(kCallbackNameNotifyThrottling);
    EXPECT_TRUE(res.no_timeout);
    ASSERT_TRUE(res.args);
    EXPECT_EQ(kThrottleTemp, res.args->temperature);
}

// Test Thermal->registerThermalChangedCallback.
TEST_P(ThermalHidlWrapperTest, RegisterThermalChangedCallbackTest) {
    // Expect to fail with same callback
    auto ret = mThermal->registerThermalChangedCallback(
            mThermalCallback, false, TemperatureType::SKIN,
            [](ThermalStatus status) { EXPECT_EQ(ThermalStatusCode::FAILURE, status.code); });
    ASSERT_TRUE(ret.isOk());
    // Expect to fail with null callback
    ret = mThermal->registerThermalChangedCallback(
            nullptr, false, TemperatureType::SKIN,
            [](ThermalStatus status) { EXPECT_EQ(ThermalStatusCode::FAILURE, status.code); });
    ASSERT_TRUE(ret.isOk());
    sp<ThermalCallback> localThermalCallback = new (std::nothrow) ThermalCallback();
    // Expect to succeed with different callback
    ret = mThermal->registerThermalChangedCallback(
            localThermalCallback, false, TemperatureType::SKIN,
            [](ThermalStatus status) { EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code); });
    ASSERT_TRUE(ret.isOk());
    // Remove the local callback
    ret = mThermal->unregisterThermalChangedCallback(
            localThermalCallback,
            [](ThermalStatus status) { EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code); });
    ASSERT_TRUE(ret.isOk());
    // Expect to fail with null callback
    ret = mThermal->unregisterThermalChangedCallback(nullptr, [](ThermalStatus status) {
        EXPECT_EQ(ThermalStatusCode::FAILURE, status.code);
    });
    ASSERT_TRUE(ret.isOk());
}

// Test Thermal->unregisterThermalChangedCallback.
TEST_P(ThermalHidlWrapperTest, UnregisterThermalChangedCallbackTest) {
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
TEST_P(ThermalHidlWrapperTest, TemperatureTest) {
    mThermal->getCurrentTemperatures(false, TemperatureType::SKIN,
                                     [](ThermalStatus status, hidl_vec<Temperature> temperatures) {
                                         if (temperatures.size()) {
                                             EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
                                         } else {
                                             EXPECT_NE(ThermalStatusCode::SUCCESS, status.code);
                                         }
                                         for (int i = 0; i < temperatures.size(); ++i) {
                                             EXPECT_LT(0u, temperatures[i].name.size());
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
                        EXPECT_LT(0u, temperatures[i].name.size());
                    }
                });
    }
}

// Sanity test for Thermal::getTemperatureThresholds().
TEST_P(ThermalHidlWrapperTest, TemperatureThresholdTest) {
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
                true, type,
                [&type](ThermalStatus status, hidl_vec<TemperatureThreshold> temperatures) {
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
TEST_P(ThermalHidlWrapperTest, CoolingDeviceTest) {
    mThermal->getCurrentCoolingDevices(
            false, CoolingType::CPU,
            [](ThermalStatus status, hidl_vec<CoolingDevice> cooling_devices) {
                if (cooling_devices.size()) {
                    EXPECT_EQ(ThermalStatusCode::SUCCESS, status.code);
                } else {
                    EXPECT_NE(ThermalStatusCode::SUCCESS, status.code);
                }
                for (int i = 0; i < cooling_devices.size(); ++i) {
                    EXPECT_LT(0u, cooling_devices[i].name.size());
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
                        EXPECT_LT(0u, cooling_devices[i].name.size());
                    }
                });
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ThermalHidlWrapperTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, ThermalHidlWrapperTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IThermal::descriptor)),
        ::android::hardware::PrintInstanceNameToString);

}  // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}

}  // namespace aidl::android::hardware::thermal
