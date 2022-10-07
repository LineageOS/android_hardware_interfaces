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
#include <vector>

#define LOG_TAG "thermal_aidl_hal_test"

#include <aidl/Gtest.h>
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

// The main test class for THERMAL HIDL HAL.
class ThermalAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        AIBinder* binder = AServiceManager_waitForService(GetParam().c_str());
        ASSERT_NE(binder, nullptr);
        mThermal = IThermal::fromBinder(ndk::SpAIBinder(binder));

        mThermalCallback = ndk::SharedRefBase::make<ThermalCallback>();
        ASSERT_NE(mThermalCallback, nullptr);
        auto ret = mThermal->registerThermalChangedCallback(mThermalCallback);
        ASSERT_TRUE(ret.isOk());
        // Expect to fail if register again
        ret = mThermal->registerThermalChangedCallback(mThermalCallback);
        ASSERT_FALSE(ret.isOk());
        ASSERT_TRUE(ret.getStatus() == STATUS_INVALID_OPERATION);
    }

    void TearDown() override {
        auto ret = mThermal->unregisterThermalChangedCallback(mThermalCallback);
        ASSERT_TRUE(ret.isOk());
        // Expect to fail if unregister again
        ret = mThermal->unregisterThermalChangedCallback(mThermalCallback);
        ASSERT_FALSE(ret.isOk());
        ASSERT_TRUE(ret.getStatus() == STATUS_INVALID_OPERATION);
    }

  protected:
    std::shared_ptr<IThermal> mThermal;
    std::shared_ptr<ThermalCallback> mThermalCallback;
};

// Test ThermalChangedCallback::notifyThrottling().
// This just calls into and back from our local ThermalChangedCallback impl.
TEST_P(ThermalAidlTest, NotifyThrottlingTest) {
    std::shared_ptr<ThermalCallback> thermalCallback = ndk::SharedRefBase::make<ThermalCallback>();
    auto ret = thermalCallback->notifyThrottling(kThrottleTemp);
    ASSERT_TRUE(ret.isOk());
    ASSERT_TRUE(thermalCallback->waitForCallback(200ms));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ThermalAidlTest);
INSTANTIATE_TEST_SUITE_P(
        Thermal, ThermalAidlTest,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IThermal::descriptor)),
        ::android::PrintInstanceNameToString);

}  // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}

}  // namespace aidl::android::hardware::thermal
