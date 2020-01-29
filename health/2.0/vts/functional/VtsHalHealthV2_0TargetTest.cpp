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

#define LOG_TAG "health_hidl_hal_test"

#include <chrono>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/hardware/health/1.0/types.h>
#include <android/hardware/health/2.0/IHealth.h>
#include <android/hardware/health/2.0/types.h>
#include <gflags/gflags.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>
#include <log/log.h>

using ::testing::AssertionFailure;
using ::testing::AssertionResult;
using ::testing::AssertionSuccess;
using namespace std::chrono_literals;

DEFINE_bool(force, false, "Force test healthd even when the default instance is present.");

// Return expr if it is evaluated to false.
#define TEST_AND_RETURN(expr) \
    do {                      \
        auto res = (expr);    \
        if (!res) return res; \
    } while (0)

namespace android {
namespace hardware {
namespace health {

using V1_0::BatteryStatus;
using V1_0::toString;

namespace V2_0 {

class HealthHidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        std::string serviceName = GetParam();

        if (serviceName == "backup" && !FLAGS_force &&
            IHealth::getService() != nullptr) {
            LOG(INFO) << "Skipping tests on healthd because the default instance is present. "
                      << "Use --force if you really want to test healthd.";
            GTEST_SKIP();
        }

        LOG(INFO) << "get service with name:" << serviceName;
        ASSERT_FALSE(serviceName.empty());
        mHealth = IHealth::getService(serviceName);
        ASSERT_NE(mHealth, nullptr);
    }

    sp<IHealth> mHealth;
};

class Callback : public IHealthInfoCallback {
   public:
    Return<void> healthInfoChanged(const HealthInfo&) override {
        std::lock_guard<std::mutex> lock(mMutex);
        mInvoked = true;
        mInvokedNotify.notify_all();
        return Void();
    }
    template <typename R, typename P>
    bool waitInvoke(std::chrono::duration<R, P> duration) {
        std::unique_lock<std::mutex> lock(mMutex);
        bool r = mInvokedNotify.wait_for(lock, duration, [this] { return this->mInvoked; });
        mInvoked = false;
        return r;
    }
   private:
    std::mutex mMutex;
    std::condition_variable mInvokedNotify;
    bool mInvoked = false;
};

#define ASSERT_OK(r) ASSERT_TRUE(isOk(r))
#define EXPECT_OK(r) EXPECT_TRUE(isOk(r))
template <typename T>
AssertionResult isOk(const Return<T>& r) {
    return r.isOk() ? AssertionSuccess() : (AssertionFailure() << r.description());
}

#define ASSERT_ALL_OK(r) ASSERT_TRUE(isAllOk(r))
// Both isOk() and Result::SUCCESS
AssertionResult isAllOk(const Return<Result>& r) {
    if (!r.isOk()) {
        return AssertionFailure() << r.description();
    }
    if (static_cast<Result>(r) != Result::SUCCESS) {
        return AssertionFailure() << toString(static_cast<Result>(r));
    }
    return AssertionSuccess();
}

/**
 * Test whether callbacks work. Tested functions are IHealth::registerCallback,
 * unregisterCallback, and update.
 */
TEST_P(HealthHidlTest, Callbacks) {
    using namespace std::chrono_literals;
    sp<Callback> firstCallback = new Callback();
    sp<Callback> secondCallback = new Callback();

    ASSERT_ALL_OK(mHealth->registerCallback(firstCallback));
    ASSERT_ALL_OK(mHealth->registerCallback(secondCallback));

    // registerCallback may or may not invoke the callback immediately, so the test needs
    // to wait for the invocation. If the implementation chooses not to invoke the callback
    // immediately, just wait for some time.
    firstCallback->waitInvoke(200ms);
    secondCallback->waitInvoke(200ms);

    // assert that the first callback is invoked when update is called.
    ASSERT_ALL_OK(mHealth->update());

    ASSERT_TRUE(firstCallback->waitInvoke(1s));
    ASSERT_TRUE(secondCallback->waitInvoke(1s));

    ASSERT_ALL_OK(mHealth->unregisterCallback(firstCallback));

    // clear any potentially pending callbacks result from wakealarm / kernel events
    // If there is none, just wait for some time.
    firstCallback->waitInvoke(200ms);
    secondCallback->waitInvoke(200ms);

    // assert that the second callback is still invoked even though the first is unregistered.
    ASSERT_ALL_OK(mHealth->update());

    ASSERT_FALSE(firstCallback->waitInvoke(200ms));
    ASSERT_TRUE(secondCallback->waitInvoke(1s));

    ASSERT_ALL_OK(mHealth->unregisterCallback(secondCallback));
}

TEST_P(HealthHidlTest, UnregisterNonExistentCallback) {
    sp<Callback> callback = new Callback();
    auto ret = mHealth->unregisterCallback(callback);
    ASSERT_OK(ret);
    ASSERT_EQ(Result::NOT_FOUND, static_cast<Result>(ret)) << "Actual: " << toString(ret);
}

/**
 * Pass the test if:
 *  - Property is not supported (res == NOT_SUPPORTED)
 *  - Result is success, and predicate is true
 * @param res the Result value.
 * @param valueStr the string representation for actual value (for error message)
 * @param pred a predicate that test whether the value is valid
 */
#define EXPECT_VALID_OR_UNSUPPORTED_PROP(res, valueStr, pred) \
    EXPECT_TRUE(isPropertyOk(res, valueStr, pred, #pred))

AssertionResult isPropertyOk(Result res, const std::string& valueStr, bool pred,
                             const std::string& predStr) {
    if (res == Result::SUCCESS) {
        if (pred) {
            return AssertionSuccess();
        }
        return AssertionFailure() << "value doesn't match.\nActual: " << valueStr
                                  << "\nExpected: " << predStr;
    }
    if (res == Result::NOT_SUPPORTED) {
        return AssertionSuccess();
    }
    return AssertionFailure() << "Result is not SUCCESS or NOT_SUPPORTED: " << toString(res);
}

bool verifyStorageInfo(const hidl_vec<struct StorageInfo>& info) {
    for (size_t i = 0; i < info.size(); i++) {
        if (!(0 <= info[i].eol && info[i].eol <= 3 && 0 <= info[i].lifetimeA &&
              info[i].lifetimeA <= 0x0B && 0 <= info[i].lifetimeB && info[i].lifetimeB <= 0x0B)) {
            return false;
        }
    }

    return true;
}

template <typename T>
bool verifyEnum(T value) {
    for (auto it : hidl_enum_range<T>()) {
        if (it == value) {
            return true;
        }
    }

    return false;
}

bool verifyHealthInfo(const HealthInfo& health_info) {
    if (!verifyStorageInfo(health_info.storageInfos)) {
        return false;
    }

    using V1_0::BatteryStatus;
    using V1_0::BatteryHealth;

    if (!((health_info.legacy.batteryCurrent != INT32_MIN) &&
          (0 <= health_info.legacy.batteryLevel && health_info.legacy.batteryLevel <= 100) &&
          verifyEnum<BatteryHealth>(health_info.legacy.batteryHealth) &&
          verifyEnum<BatteryStatus>(health_info.legacy.batteryStatus))) {
        return false;
    }

    if (health_info.legacy.batteryPresent) {
        // If a battery is present, the battery status must be known.
        if (!((health_info.legacy.batteryChargeCounter > 0) &&
              (health_info.legacy.batteryStatus != BatteryStatus::UNKNOWN))) {
            return false;
        }
    }

    return true;
}

/*
 * Tests the values returned by getChargeCounter() from interface IHealth.
 */
TEST_P(HealthHidlTest, getChargeCounter) {
    EXPECT_OK(mHealth->getChargeCounter([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value > 0);
    }));
}

/*
 * Tests the values returned by getCurrentNow() from interface IHealth.
 */
TEST_P(HealthHidlTest, getCurrentNow) {
    EXPECT_OK(mHealth->getCurrentNow([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value != INT32_MIN);
    }));
}

/*
 * Tests the values returned by getCurrentAverage() from interface IHealth.
 */
TEST_P(HealthHidlTest, getCurrentAverage) {
    EXPECT_OK(mHealth->getCurrentAverage([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value != INT32_MIN);
    }));
}

/*
 * Tests the values returned by getCapacity() from interface IHealth.
 */
TEST_P(HealthHidlTest, getCapacity) {
    EXPECT_OK(mHealth->getCapacity([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), 0 <= value && value <= 100);
    }));
}

/*
 * Tests the values returned by getEnergyCounter() from interface IHealth.
 */
TEST_P(HealthHidlTest, getEnergyCounter) {
    EXPECT_OK(mHealth->getEnergyCounter([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value != INT64_MIN);
    }));
}

/*
 * Tests the values returned by getChargeStatus() from interface IHealth.
 */
TEST_P(HealthHidlTest, getChargeStatus) {
    EXPECT_OK(mHealth->getChargeStatus([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, toString(value), verifyEnum<BatteryStatus>(value));
    }));
}

/*
 * Tests the values returned by getStorageInfo() from interface IHealth.
 */
TEST_P(HealthHidlTest, getStorageInfo) {
    EXPECT_OK(mHealth->getStorageInfo([](auto result, auto& value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, toString(value), verifyStorageInfo(value));
    }));
}

/*
 * Tests the values returned by getDiskStats() from interface IHealth.
 */
TEST_P(HealthHidlTest, getDiskStats) {
    EXPECT_OK(mHealth->getDiskStats([](auto result, auto& value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, toString(value), true);
    }));
}

/*
 * Tests the values returned by getHealthInfo() from interface IHealth.
 */
TEST_P(HealthHidlTest, getHealthInfo) {
    EXPECT_OK(mHealth->getHealthInfo([](auto result, auto& value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, toString(value), verifyHealthInfo(value));
    }));
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, HealthHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IHealth::descriptor)),
        android::hardware::PrintInstanceNameToString);

// For battery current tests, value may not be stable if the battery current has fluctuated.
// Retry in a bit more time (with the following timeout) and consider the test successful if it
// has succeed once.
static constexpr auto gBatteryTestTimeout = 1min;
// Tests on battery current signs are only enforced on devices launching with Android 11.
static constexpr int64_t gBatteryTestMinShippingApiLevel = 30;
static constexpr double gCurrentCompareFactor = 0.50;

// Tuple for all IHealth::get* API return values.
template <typename T>
struct HalResult {
    Result result;
    T value;
};

// Needs to be called repeatedly within a period of time to ensure values are initialized.
static AssertionResult IsBatteryCurrentSignCorrect(HalResult<BatteryStatus> status,
                                                   HalResult<int32_t> current,
                                                   bool acceptZeroCurrentAsUnknown) {
    // getChargeStatus / getCurrentNow / getCurrentAverage / getHealthInfo already tested above.
    // Here, just skip if not ok.
    if (status.result != Result::SUCCESS) {
        return AssertionSuccess() << "getChargeStatus / getHealthInfo returned "
                                  << toString(status.result) << ", skipping";
    }

    if (current.result != Result::SUCCESS) {
        return AssertionSuccess() << "getCurrentNow / getCurrentAverage returned "
                                  << toString(current.result) << ", skipping";
    }

    // For IHealth.getCurrentNow/Average, if current is not available, it is expected that
    // current.result == Result::NOT_SUPPORTED, which is checked above. Hence, zero current is
    // not treated as unknown values.
    // For IHealth.getHealthInfo, if current is not available, health_info.current_* == 0.
    // Caller of this function provides current.result == Result::SUCCESS. Hence, just skip the
    // check.
    if (current.value == 0 && acceptZeroCurrentAsUnknown) {
        return AssertionSuccess()
               << "current is 0, which indicates the value may not be available. Skipping.";
    }

    switch (status.value) {
        case BatteryStatus::UNKNOWN:
            if (current.value != 0) {
                // BatteryStatus may be UNKNOWN initially with a non-zero current value, but
                // after it is initialized, it should be known.
                return AssertionFailure()
                       << "BatteryStatus is UNKNOWN but current is not 0. Actual: "
                       << current.value;
            }
            break;
        case BatteryStatus::CHARGING:
            if (current.value <= 0) {
                return AssertionFailure()
                       << "BatteryStatus is CHARGING but current is not positive. Actual: "
                       << current.value;
            }
            break;
        case BatteryStatus::NOT_CHARGING:
            if (current.value > 0) {
                return AssertionFailure() << "BatteryStatus is " << toString(status.value)
                                          << " but current is positive. Actual: " << current.value;
            }
            break;
        case BatteryStatus::DISCHARGING:
            if (current.value >= 0) {
                return AssertionFailure()
                       << "BatteryStatus is " << toString(status.value)
                       << " but current is not negative. Actual: " << current.value;
            }
            break;
        case BatteryStatus::FULL:
            // Battery current may be positive or negative depending on the load.
            break;
        default:
            return AssertionFailure() << "Unknown BatteryStatus " << toString(status.value);
    }

    return AssertionSuccess() << "BatteryStatus is " << toString(status.value)
                              << " and current has the correct sign: " << current.value;
}

static AssertionResult IsValueSimilar(int32_t dividend, int32_t divisor, double factor) {
    auto difference = abs(dividend - divisor);
    if (difference > factor * abs(divisor)) {
        return AssertionFailure() << dividend << " and " << divisor << " are not similar.";
    }
    return AssertionSuccess() << dividend << " and " << divisor << " are similar.";
}

static AssertionResult IsBatteryCurrentSimilar(HalResult<BatteryStatus> status,
                                               HalResult<int32_t> currentNow,
                                               HalResult<int32_t> currentAverage) {
    if (status.result == Result::SUCCESS && status.value == BatteryStatus::FULL) {
        // No reason to test on full battery because battery current load fluctuates.
        return AssertionSuccess() << "Battery is full, skipping";
    }

    // getCurrentNow / getCurrentAverage / getHealthInfo already tested above. Here, just skip if
    // not SUCCESS or value 0.
    if (currentNow.result != Result::SUCCESS || currentNow.value == 0) {
        return AssertionSuccess() << "getCurrentNow returned " << toString(currentNow.result)
                                  << " with value " << currentNow.value << ", skipping";
    }

    if (currentAverage.result != Result::SUCCESS || currentAverage.value == 0) {
        return AssertionSuccess() << "getCurrentAverage returned "
                                  << toString(currentAverage.result) << " with value "
                                  << currentAverage.value << ", skipping";
    }

    // Check that the two values are similar. Note that the two tests uses a different
    // divisor to ensure that they are actually pretty similar. For example,
    // IsValueSimilar(5,10,0.4) returns true, but IsValueSimlar(10,5,0.4) returns false.
    TEST_AND_RETURN(IsValueSimilar(currentNow.value, currentAverage.value, gCurrentCompareFactor)
                    << " for now vs. average. Check units.");
    TEST_AND_RETURN(IsValueSimilar(currentAverage.value, currentNow.value, gCurrentCompareFactor)
                    << " for average vs. now. Check units.");
    return AssertionSuccess() << "currentNow = " << currentNow.value
                              << " and currentAverage = " << currentAverage.value
                              << " are considered similar.";
}

// Test that f() returns AssertionSuccess() once in a given period of time.
template <typename Duration, typename Function>
static AssertionResult SucceedOnce(Duration d, Function f) {
    AssertionResult result = AssertionFailure() << "Function never evaluated.";
    auto end = std::chrono::system_clock::now() + d;
    while (std::chrono::system_clock::now() <= end) {
        result = f();
        if (result) {
            return result;
        }
        std::this_thread::sleep_for(2s);
    }
    return result;
}

uint64_t GetShippingApiLevel() {
    uint64_t api_level = android::base::GetUintProperty<uint64_t>("ro.product.first_api_level", 0);
    if (api_level != 0) {
        return api_level;
    }
    return android::base::GetUintProperty<uint64_t>("ro.build.version.sdk", 0);
}

class BatteryTest : public HealthHidlTest {
  public:
    void SetUp() override {
        HealthHidlTest::SetUp();

        auto shippingApiLevel = GetShippingApiLevel();
        if (shippingApiLevel < gBatteryTestMinShippingApiLevel) {
            GTEST_SKIP() << "Skipping on devices with first API level " << shippingApiLevel;
        }
    }
};

TEST_P(BatteryTest, InstantCurrentAgainstChargeStatusInHealthInfo) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<HealthInfo> healthInfo;
        TEST_AND_RETURN(isOk(mHealth->getHealthInfo([&](auto result, const auto& value) {
            healthInfo = {result, value};
        })));

        return IsBatteryCurrentSignCorrect(
                {healthInfo.result, healthInfo.value.legacy.batteryStatus},
                {healthInfo.result, healthInfo.value.legacy.batteryCurrent},
                true /* accept zero current as unknown */);
    };
    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when current_now becomes stable.";
}

TEST_P(BatteryTest, AverageCurrentAgainstChargeStatusInHealthInfo) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<HealthInfo> healthInfo;
        TEST_AND_RETURN(isOk(mHealth->getHealthInfo([&](auto result, const auto& value) {
            healthInfo = {result, value};
        })));
        return IsBatteryCurrentSignCorrect(
                {healthInfo.result, healthInfo.value.legacy.batteryStatus},
                {healthInfo.result, healthInfo.value.batteryCurrentAverage},
                true /* accept zero current as unknown */);
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when current_average becomes stable.";
}

TEST_P(BatteryTest, InstantCurrentAgainstAverageCurrentInHealthInfo) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<HealthInfo> healthInfo;
        TEST_AND_RETURN(isOk(mHealth->getHealthInfo([&](auto result, const auto& value) {
            healthInfo = {result, value};
        })));
        return IsBatteryCurrentSimilar({healthInfo.result, healthInfo.value.legacy.batteryStatus},
                                       {healthInfo.result, healthInfo.value.legacy.batteryCurrent},
                                       {healthInfo.result, healthInfo.value.batteryCurrentAverage});
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when current_now and current_average becomes "
               "stable.";
}

TEST_P(BatteryTest, InstantCurrentAgainstChargeStatusFromHal) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<BatteryStatus> status;
        HalResult<int32_t> currentNow;
        TEST_AND_RETURN(isOk(mHealth->getChargeStatus([&](auto result, auto value) {
            status = {result, value};
        })));
        TEST_AND_RETURN(isOk(mHealth->getCurrentNow([&](auto result, auto value) {
            currentNow = {result, value};
        })));

        return IsBatteryCurrentSignCorrect(status, currentNow,
                                           false /* accept zero current as unknown */);
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when current_now becomes stable.";
}

TEST_P(BatteryTest, AverageCurrentAgainstChargeStatusFromHal) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<BatteryStatus> status;
        TEST_AND_RETURN(isOk(mHealth->getChargeStatus([&](auto result, auto value) {
            status = {result, value};
        })));
        HalResult<int32_t> currentAverage;
        TEST_AND_RETURN(isOk(mHealth->getCurrentAverage([&](auto result, auto value) {
            currentAverage = {result, value};
        })));
        return IsBatteryCurrentSignCorrect(status, currentAverage,
                                           false /* accept zero current as unknown */);
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when current_average becomes stable.";
}

TEST_P(BatteryTest, InstantCurrentAgainstAverageCurrentFromHal) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<BatteryStatus> status;
        TEST_AND_RETURN(isOk(mHealth->getChargeStatus([&](auto result, auto value) {
            status = {result, value};
        })));
        HalResult<int32_t> currentNow;
        TEST_AND_RETURN(isOk(mHealth->getCurrentNow([&](auto result, auto value) {
            currentNow = {result, value};
        })));
        HalResult<int32_t> currentAverage;
        TEST_AND_RETURN(isOk(mHealth->getCurrentAverage([&](auto result, auto value) {
            currentAverage = {result, value};
        })));
        return IsBatteryCurrentSimilar(status, currentNow, currentAverage);
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when current_average becomes stable.";
}

AssertionResult IsBatteryStatusCorrect(HalResult<BatteryStatus> status,
                                       HalResult<HealthInfo> healthInfo) {
    // getChargetStatus / getHealthInfo is already tested above. Here, just skip if not ok.
    if (healthInfo.result != Result::SUCCESS) {
        return AssertionSuccess() << "getHealthInfo returned " << toString(healthInfo.result)
                                  << ", skipping";
    }
    if (status.result != Result::SUCCESS) {
        return AssertionSuccess() << "getChargeStatus returned " << toString(status.result)
                                  << ", skipping";
    }

    const auto& batteryInfo = healthInfo.value.legacy;
    bool isConnected = batteryInfo.chargerAcOnline || batteryInfo.chargerUsbOnline ||
                       batteryInfo.chargerWirelessOnline;

    std::stringstream message;
    message << "BatteryStatus is " << toString(status.value) << " and "
            << (isConnected ? "" : "no ")
            << "power source is connected: ac=" << batteryInfo.chargerAcOnline
            << ", usb=" << batteryInfo.chargerUsbOnline
            << ", wireless=" << batteryInfo.chargerWirelessOnline;

    switch (status.value) {
        case BatteryStatus::UNKNOWN: {
            // Don't enforce anything on isConnected on unknown battery status.
            // Battery-less devices must report UNKNOWN battery status, but may report true
            // or false on isConnected.
        } break;
        case BatteryStatus::CHARGING:
        case BatteryStatus::NOT_CHARGING:
        case BatteryStatus::FULL: {
            if (!isConnected) {
                return AssertionFailure() << message.str();
            }
        } break;
        case BatteryStatus::DISCHARGING: {
            if (isConnected) {
                return AssertionFailure() << message.str();
            }
        } break;
        default: {
            return AssertionFailure() << "Unknown battery status value " << toString(status.value);
        } break;
    }

    return AssertionSuccess() << message.str();
}

TEST_P(BatteryTest, ConnectedAgainstStatusFromHal) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<BatteryStatus> status;
        TEST_AND_RETURN(isOk(mHealth->getChargeStatus([&](auto result, auto value) {
            status = {result, value};
        })));
        HalResult<HealthInfo> healthInfo;
        TEST_AND_RETURN(isOk(mHealth->getHealthInfo([&](auto result, const auto& value) {
            healthInfo = {result, value};
        })));
        return IsBatteryStatusCorrect(status, healthInfo);
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when battery_status becomes stable.";
}

TEST_P(BatteryTest, ConnectedAgainstStatusInHealthInfo) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<HealthInfo> healthInfo;
        TEST_AND_RETURN(isOk(mHealth->getHealthInfo([&](auto result, const auto& value) {
            healthInfo = {result, value};
        })));
        return IsBatteryStatusCorrect({healthInfo.result, healthInfo.value.legacy.batteryStatus},
                                      healthInfo);
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when getHealthInfo becomes stable.";
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, BatteryTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IHealth::descriptor)),
        android::hardware::PrintInstanceNameToString);

}  // namespace V2_0
}  // namespace health
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true /* remove flags */);
    return RUN_ALL_TESTS();
}
