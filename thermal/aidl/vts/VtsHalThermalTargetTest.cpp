/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#define LOG_TAG "thermal_aidl_hal_test"

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/thermal/BnCoolingDeviceChangedCallback.h>
#include <aidl/android/hardware/thermal/BnThermal.h>
#include <aidl/android/hardware/thermal/BnThermalChangedCallback.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/binder_ibinder.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/binder_status.h>
#include <gtest/gtest.h>

#include <unistd.h>

namespace aidl::android::hardware::thermal {

namespace {

using ::android::sp;
using android::hardware::thermal::CoolingDevice;
using android::hardware::thermal::IThermal;
using android::hardware::thermal::Temperature;
using android::hardware::thermal::TemperatureType;

using namespace std::string_literals;
using namespace std::chrono_literals;

static const Temperature kThrottleTemp = {
        .type = TemperatureType::SKIN,
        .name = "test temperature sensor",
        .value = 98.6,
        .throttlingStatus = ThrottlingSeverity::CRITICAL,
};

static const CoolingDevice kCoolingDevice = {
        .type = CoolingType::CPU,
        .name = "test cooling device",
        .value = 1,
        .powerLimitMw = 300,
        .powerMw = 500,
        .timeWindowMs = 7000,
};

// Callback class for receiving thermal event notifications from main class
class ThermalCallback : public BnThermalChangedCallback {
  public:
    ndk::ScopedAStatus notifyThrottling(const Temperature&) override {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mInvoke = true;
        }
        mNotifyThrottling.notify_all();
        return ndk::ScopedAStatus::ok();
    }

    template <typename R, typename P>
    [[nodiscard]] bool waitForCallback(std::chrono::duration<R, P> duration) {
        std::unique_lock<std::mutex> lock(mMutex);
        bool r = mNotifyThrottling.wait_for(lock, duration, [this] { return this->mInvoke; });
        mInvoke = false;
        return r;
    }

  private:
    std::mutex mMutex;
    std::condition_variable mNotifyThrottling;
    bool mInvoke = false;
};

// Callback class for receiving cooling device event notifications from main class
class CoolingDeviceCallback : public BnCoolingDeviceChangedCallback {
  public:
    ndk::ScopedAStatus notifyCoolingDeviceChanged(const CoolingDevice&) override {
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mInvoke = true;
        }
        mNotifyCoolingDeviceChanged.notify_all();
        return ndk::ScopedAStatus::ok();
    }

    template <typename R, typename P>
    [[nodiscard]] bool waitForCallback(std::chrono::duration<R, P> duration) {
        std::unique_lock<std::mutex> lock(mMutex);
        bool r = mNotifyCoolingDeviceChanged.wait_for(lock, duration,
                                                      [this] { return this->mInvoke; });
        mInvoke = false;
        return r;
    }

  private:
    std::mutex mMutex;
    std::condition_variable mNotifyCoolingDeviceChanged;
    bool mInvoke = false;
};

// The main test class for THERMAL HIDL HAL.
class ThermalAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        AIBinder* binder = AServiceManager_waitForService(GetParam().c_str());
        ASSERT_NE(binder, nullptr);
        mThermal = IThermal::fromBinder(ndk::SpAIBinder(binder));

        mThermalCallback = ndk::SharedRefBase::make<ThermalCallback>();
        ASSERT_NE(mThermalCallback, nullptr);
        ::ndk::ScopedAStatus status = mThermal->registerThermalChangedCallback(mThermalCallback);
        ASSERT_TRUE(status.isOk()) << status.getMessage();

        auto ret = mThermal->getInterfaceVersion(&thermal_version);
        ASSERT_TRUE(ret.isOk()) << ret;
        if (thermal_version > 1) {
            mCoolingDeviceCallback = ndk::SharedRefBase::make<CoolingDeviceCallback>();
            ASSERT_NE(mCoolingDeviceCallback, nullptr);
            status = mThermal->registerCoolingDeviceChangedCallbackWithType(mCoolingDeviceCallback,
                                                                            kCoolingDevice.type);
            ASSERT_TRUE(status.isOk()) << status.getMessage();
        }
    }

    void TearDown() override {
        ::ndk::ScopedAStatus status = mThermal->unregisterThermalChangedCallback(mThermalCallback);
        ASSERT_TRUE(status.isOk()) << status.getMessage();

        // Expect to fail if unregister again
        status = mThermal->unregisterThermalChangedCallback(mThermalCallback);
        ASSERT_EQ(EX_ILLEGAL_ARGUMENT, status.getExceptionCode());

        auto ret = mThermal->getInterfaceVersion(&thermal_version);
        ASSERT_TRUE(ret.isOk()) << ret;
        if (thermal_version > 1) {
            status = mThermal->unregisterCoolingDeviceChangedCallback(mCoolingDeviceCallback);
            ASSERT_TRUE(status.isOk()) << status.getMessage();
            status = mThermal->unregisterCoolingDeviceChangedCallback(mCoolingDeviceCallback);
            ASSERT_EQ(EX_ILLEGAL_ARGUMENT, status.getExceptionCode());
        }
    }
    // Stores thermal version
    int32_t thermal_version;

  protected:
    std::shared_ptr<IThermal> mThermal;
    std::shared_ptr<ThermalCallback> mThermalCallback;
    std::shared_ptr<CoolingDeviceCallback> mCoolingDeviceCallback;
};

// Test ThermalChangedCallback::notifyThrottling().
// This just calls into and back from our local ThermalChangedCallback impl.
TEST_P(ThermalAidlTest, NotifyThrottlingTest) {
    std::shared_ptr<ThermalCallback> thermalCallback = ndk::SharedRefBase::make<ThermalCallback>();
    ::ndk::ScopedAStatus status = thermalCallback->notifyThrottling(kThrottleTemp);
    ASSERT_TRUE(status.isOk()) << status.getMessage();
    ASSERT_TRUE(thermalCallback->waitForCallback(200ms));
}

// Test CoolingDeviceChangedCallback::notifyCoolingDeviceChanged().
// This just calls into and back from our local CoolingDeviceChangedCallback impl.
TEST_P(ThermalAidlTest, NotifyCoolingDeviceChangedTest) {
    auto ret = mThermal->getInterfaceVersion(&thermal_version);
    ASSERT_TRUE(ret.isOk()) << ret;
    if (thermal_version < 2) {
        return;
    }
    std::shared_ptr<CoolingDeviceCallback> cdevCallback =
            ndk::SharedRefBase::make<CoolingDeviceCallback>();
    ::ndk::ScopedAStatus status = cdevCallback->notifyCoolingDeviceChanged(kCoolingDevice);
    ASSERT_TRUE(status.isOk()) << status.getMessage();
    ASSERT_TRUE(cdevCallback->waitForCallback(200ms));
}

// Test Thermal->registerThermalChangedCallback.
TEST_P(ThermalAidlTest, RegisterThermalChangedCallbackTest) {
    // Expect to fail with same callback
    ::ndk::ScopedAStatus status = mThermal->registerThermalChangedCallback(mThermalCallback);
    ASSERT_EQ(EX_ILLEGAL_ARGUMENT, status.getExceptionCode());
    // Expect to fail with null callback
    status = mThermal->registerThermalChangedCallback(nullptr);
    ASSERT_TRUE(status.getExceptionCode() == EX_ILLEGAL_ARGUMENT
        || status.getExceptionCode() == EX_NULL_POINTER);
    std::shared_ptr<ThermalCallback> localThermalCallback =
            ndk::SharedRefBase::make<ThermalCallback>();
    // Expect to succeed with different callback
    status = mThermal->registerThermalChangedCallback(localThermalCallback);
    ASSERT_TRUE(status.isOk()) << status.getMessage();
    // Remove the local callback
    status = mThermal->unregisterThermalChangedCallback(localThermalCallback);
    ASSERT_TRUE(status.isOk()) << status.getMessage();
    // Expect to fail with null callback
    status = mThermal->unregisterThermalChangedCallback(nullptr);
    ASSERT_TRUE(status.getExceptionCode() == EX_ILLEGAL_ARGUMENT
        || status.getExceptionCode() == EX_NULL_POINTER);
}

// Test Thermal->registerThermalChangedCallbackWithType.
TEST_P(ThermalAidlTest, RegisterThermalChangedCallbackWithTypeTest) {
    // Expect to fail with same callback
    ::ndk::ScopedAStatus status = mThermal->registerThermalChangedCallbackWithType(
            mThermalCallback, TemperatureType::SKIN);
    ASSERT_EQ(EX_ILLEGAL_ARGUMENT, status.getExceptionCode());
    // Expect to fail with null callback
    status = mThermal->registerThermalChangedCallbackWithType(nullptr, TemperatureType::SKIN);
    ASSERT_TRUE(status.getExceptionCode() == EX_ILLEGAL_ARGUMENT
        || status.getExceptionCode() == EX_NULL_POINTER);
    std::shared_ptr<ThermalCallback> localThermalCallback =
            ndk::SharedRefBase::make<ThermalCallback>();
    // Expect to succeed with different callback
    status = mThermal->registerThermalChangedCallbackWithType(localThermalCallback,
                                                              TemperatureType::SKIN);
    ASSERT_TRUE(status.isOk()) << status.getMessage();
    // Remove the local callback
    status = mThermal->unregisterThermalChangedCallback(localThermalCallback);
    ASSERT_TRUE(status.isOk()) << status.getMessage();
    // Expect to fail with null callback
    status = mThermal->unregisterThermalChangedCallback(nullptr);
    ASSERT_TRUE(status.getExceptionCode() == EX_ILLEGAL_ARGUMENT
        || status.getExceptionCode() == EX_NULL_POINTER);
}

// Test Thermal->registerCoolingDeviceChangedCallbackWithType.
TEST_P(ThermalAidlTest, RegisterCoolingDeviceChangedCallbackWithTypeTest) {
    auto ret = mThermal->getInterfaceVersion(&thermal_version);
    ASSERT_TRUE(ret.isOk()) << ret;
    if (thermal_version < 2) {
        return;
    }

    // Expect to fail with same callback
    ::ndk::ScopedAStatus status = mThermal->registerCoolingDeviceChangedCallbackWithType(
            mCoolingDeviceCallback, CoolingType::CPU);
    ASSERT_EQ(EX_ILLEGAL_ARGUMENT, status.getExceptionCode());
    // Expect to fail with null callback
    status = mThermal->registerCoolingDeviceChangedCallbackWithType(nullptr, CoolingType::CPU);
    ASSERT_TRUE(status.getExceptionCode() == EX_ILLEGAL_ARGUMENT ||
                status.getExceptionCode() == EX_NULL_POINTER);
    std::shared_ptr<CoolingDeviceCallback> localCoolingDeviceCallback =
            ndk::SharedRefBase::make<CoolingDeviceCallback>();
    // Expect to succeed with different callback
    status = mThermal->registerCoolingDeviceChangedCallbackWithType(localCoolingDeviceCallback,
                                                                    CoolingType::CPU);
    ASSERT_TRUE(status.isOk()) << status.getMessage();
    // Remove the local callback
    status = mThermal->unregisterCoolingDeviceChangedCallback(localCoolingDeviceCallback);
    ASSERT_TRUE(status.isOk()) << status.getMessage();
    // Expect to fail with null callback
    status = mThermal->unregisterCoolingDeviceChangedCallback(nullptr);
    ASSERT_TRUE(status.getExceptionCode() == EX_ILLEGAL_ARGUMENT ||
                status.getExceptionCode() == EX_NULL_POINTER);
}

// Test Thermal->getCurrentTemperatures().
TEST_P(ThermalAidlTest, TemperatureTest) {
    std::vector<Temperature> ret;
    ::ndk::ScopedAStatus status = mThermal->getTemperatures(&ret);
    if (status.isOk()) {
        for (auto& i : ret) {
            EXPECT_LT(0u, i.name.size());
            LOG(INFO) << i.name + " " + toString(i.type) << "\n";
        }
    } else {
        ASSERT_EQ(EX_ILLEGAL_STATE, status.getExceptionCode());
    }

    auto types = ::ndk::enum_range<TemperatureType>();
    for (const auto& type : types) {
        status = mThermal->getTemperaturesWithType(type, &ret);

        if (status.isOk()) {
            for (auto& i : ret) {
                EXPECT_EQ(type, i.type) << "Expect type " + toString(type) + " but got " +
                                                   toString(i.type) + " for " + i.name;
                EXPECT_LT(0u, i.name.size());
            }
        } else {
            ASSERT_EQ(EX_ILLEGAL_STATE, status.getExceptionCode());
        }
    }
}

// Test Thermal->getTemperatureThresholds().
TEST_P(ThermalAidlTest, TemperatureThresholdTest) {
    std::vector<TemperatureThreshold> ret;
    ::ndk::ScopedAStatus status = mThermal->getTemperatureThresholds(&ret);
    if (status.isOk()) {
        for (auto& i : ret) {
            EXPECT_LT(0u, i.name.size());
            LOG(INFO) << i.name + " " + toString(i.type) << "\n";
        }
    } else {
        ASSERT_EQ(EX_ILLEGAL_STATE, status.getExceptionCode());
    }

    auto types = ::ndk::enum_range<TemperatureType>();
    for (const auto& type : types) {
        status = mThermal->getTemperatureThresholdsWithType(type, &ret);

        if (status.isOk()) {
            for (auto& i : ret) {
                EXPECT_EQ(type, i.type) << "Expect type " + toString(type) + " but got " +
                                                   toString(i.type) + " for " + i.name;
                EXPECT_LT(0u, i.name.size());
            }
        } else {
            ASSERT_EQ(EX_ILLEGAL_STATE, status.getExceptionCode());
        }
    }
}

// Test Thermal->getTemperatureThresholdsWithType(SKIN).
// @VsrTest = GMS-VSR-3.2.5-001
// @VsrTest = VSR-3.2.5-001
// @VsrTest = GMS-VSR-3.2.5-002
// @VsrTest = VSR-3.2.5-002
TEST_P(ThermalAidlTest, SkinTemperatureThresholdsTest) {
    auto apiLevel = ::android::base::GetIntProperty<int32_t>("ro.vendor.api_level", 0);
    if (apiLevel < 35) {
        GTEST_SKIP() << "Skipping test as the vendor level is below 35: " << apiLevel;
    }
    std::vector<Temperature> temperatures;
    ::ndk::ScopedAStatus status =
            mThermal->getTemperaturesWithType(TemperatureType::SKIN, &temperatures);
    ASSERT_TRUE(status.isOk()) << "getTemperaturesWithType(SKIN) failed";
    ASSERT_FALSE(temperatures.empty()) << "getTemperaturesWithType(SKIN) returns empty";
    ASSERT_EQ(1, temperatures.size())
            << "getTemperaturesWithType(SKIN) returns multiple temperatures";

    std::vector<TemperatureThreshold> thresholds;
    status = mThermal->getTemperatureThresholdsWithType(TemperatureType::SKIN, &thresholds);
    ASSERT_TRUE(status.isOk()) << "getTemperatureThresholdsWithType(SKIN) failed";
    ASSERT_FALSE(thresholds.empty()) << "getTemperatureThresholdsWithType(SKIN) returns empty";
    ASSERT_EQ(1, thresholds.size())
            << "getTemperatureThresholdsWithType(SKIN) returns multiple thresholds";
    auto temperature = temperatures[0];
    auto threshold = thresholds[0];
    ASSERT_EQ(temperature.name, threshold.name);
    auto severities = ::ndk::enum_range<ThrottlingSeverity>();
    auto cardinality = std::distance(severities.begin(), severities.end());
    ASSERT_NE(NAN, temperature.value);
    ASSERT_EQ(cardinality, threshold.hotThrottlingThresholds.size());
    float lastThreshold = threshold.hotThrottlingThresholds[1];
    // skip NONE, and check that the rest should be set and non-decreasing
    for (auto i = 2; i < cardinality; i++) {
        float t = threshold.hotThrottlingThresholds[i];
        ASSERT_NE(NAN, t);
        ASSERT_TRUE(t >= lastThreshold) << "Temperature thresholds should be non-decreasing "
                                        << "but got " << t << " for status " << i << " and "
                                        << lastThreshold << " for status " << i - 1;
        lastThreshold = t;
    }
}

// Test Thermal->getCoolingDevices().
TEST_P(ThermalAidlTest, CoolingDeviceTest) {
    std::vector<CoolingDevice> ret;
    ::ndk::ScopedAStatus status = mThermal->getCoolingDevices(&ret);
    if (status.isOk()) {
        for (auto& i : ret) {
            EXPECT_LT(0u, i.name.size());
            LOG(INFO) << i.name + " " + toString(i.type) << "\n";
        }
    } else {
        ASSERT_EQ(EX_ILLEGAL_STATE, status.getExceptionCode());
    }

    auto types = ::ndk::enum_range<CoolingType>();
    for (const auto& type : types) {
        status = mThermal->getCoolingDevicesWithType(type, &ret);
        if (status.isOk()) {
            ASSERT_TRUE(status.isOk());
            for (auto& i : ret) {
                EXPECT_EQ(type, i.type) << "Expect type " + toString(type) + " but got " +
                                                   toString(i.type) + " for " + i.name;
                EXPECT_LT(0u, i.name.size());
            }
        } else {
            ASSERT_EQ(EX_ILLEGAL_STATE, status.getExceptionCode());
        }
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ThermalAidlTest);
INSTANTIATE_TEST_SUITE_P(
        Thermal, ThermalAidlTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IThermal::descriptor)),
        ::android::PrintInstanceNameToString);

}  // namespace
}  // namespace aidl::android::hardware::thermal

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
