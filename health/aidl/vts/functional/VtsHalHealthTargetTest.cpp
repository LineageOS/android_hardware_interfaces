/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "health_aidl_hal_test"

#include <chrono>
#include <memory>
#include <thread>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/health/BnHealthInfoCallback.h>
#include <aidl/android/hardware/health/IHealth.h>
#include <android/binder_auto_utils.h>
#include <android/binder_enums.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <health-test/TestUtils.h>

using android::getAidlHalInstanceNames;
using android::PrintInstanceNameToString;
using android::hardware::health::test_utils::SucceedOnce;
using ndk::enum_range;
using ndk::ScopedAStatus;
using ndk::SharedRefBase;
using ndk::SpAIBinder;
using testing::AllOf;
using testing::AnyOf;
using testing::AnyOfArray;
using testing::AssertionFailure;
using testing::AssertionResult;
using testing::AssertionSuccess;
using testing::Contains;
using testing::Each;
using testing::Eq;
using testing::ExplainMatchResult;
using testing::Ge;
using testing::Gt;
using testing::Le;
using testing::Lt;
using testing::Matcher;
using testing::Not;
using namespace std::string_literals;
using namespace std::chrono_literals;

namespace aidl::android::hardware::health {

static constexpr int32_t kFullChargeDesignCapMinUah = 100 * 1000;
static constexpr int32_t kFullChargeDesignCapMaxUah = 100 * 1000 * 1000;

MATCHER(IsOk, "") {
    *result_listener << "status is " << arg.getDescription();
    return arg.isOk();
}

MATCHER_P(ExceptionIs, exception_code, "") {
    *result_listener << "status is " << arg.getDescription();
    return arg.getExceptionCode() == exception_code;
}

template <typename T>
Matcher<T> InClosedRange(const T& lo, const T& hi) {
    return AllOf(Ge(lo), Le(hi));
}

template <typename T>
Matcher<T> IsValidEnum() {
    return AnyOfArray(enum_range<T>().begin(), enum_range<T>().end());
}

MATCHER(IsValidSerialNumber, "") {
    if (!arg) {
        return true;
    }
    if (arg->size() < 6) {
        return false;
    }
    for (const auto& c : *arg) {
        if (!isalnum(c)) {
            return false;
        }
    }
    return true;
}

class HealthAidl : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
        health = IHealth::fromBinder(binder);
        ASSERT_NE(health, nullptr);
    }
    std::shared_ptr<IHealth> health;
};

class Callback : public BnHealthInfoCallback {
  public:
    ScopedAStatus healthInfoChanged(const HealthInfo&) override {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            invoked_ = true;
        }
        invoked_notify_.notify_all();
        return ScopedAStatus::ok();
    }
    template <typename R, typename P>
    [[nodiscard]] bool waitInvoke(std::chrono::duration<R, P> duration) {
        std::unique_lock<std::mutex> lock(mutex_);
        bool r = invoked_notify_.wait_for(lock, duration, [this] { return this->invoked_; });
        invoked_ = false;
        return r;
    }

  private:
    std::mutex mutex_;
    std::condition_variable invoked_notify_;
    bool invoked_ = false;
};

TEST_P(HealthAidl, Callbacks) {
    auto first_callback = SharedRefBase::make<Callback>();
    auto second_callback = SharedRefBase::make<Callback>();

    ASSERT_THAT(health->registerCallback(first_callback), IsOk());
    ASSERT_THAT(health->registerCallback(second_callback), IsOk());

    // registerCallback may or may not invoke the callback immediately, so the test needs
    // to wait for the invocation. If the implementation chooses not to invoke the callback
    // immediately, just wait for some time.
    (void)first_callback->waitInvoke(200ms);
    (void)second_callback->waitInvoke(200ms);

    // assert that the first callback is invoked when update is called.
    ASSERT_THAT(health->update(), IsOk());

    ASSERT_TRUE(first_callback->waitInvoke(1s));
    ASSERT_TRUE(second_callback->waitInvoke(1s));

    ASSERT_THAT(health->unregisterCallback(first_callback), IsOk());

    // clear any potentially pending callbacks result from wakealarm / kernel events
    // If there is none, just wait for some time.
    (void)first_callback->waitInvoke(200ms);
    (void)second_callback->waitInvoke(200ms);

    // assert that the second callback is still invoked even though the first is unregistered.
    ASSERT_THAT(health->update(), IsOk());

    ASSERT_FALSE(first_callback->waitInvoke(200ms));
    ASSERT_TRUE(second_callback->waitInvoke(1s));

    ASSERT_THAT(health->unregisterCallback(second_callback), IsOk());
}

TEST_P(HealthAidl, UnregisterNonExistentCallback) {
    auto callback = SharedRefBase::make<Callback>();
    auto ret = health->unregisterCallback(callback);
    ASSERT_THAT(ret, ExceptionIs(EX_ILLEGAL_ARGUMENT));
}

/*
 * Tests the values returned by getChargeCounterUah() from interface IHealth.
 */
TEST_P(HealthAidl, getChargeCounterUah) {
    int32_t value;
    auto status = health->getChargeCounterUah(&value);
    ASSERT_THAT(status, AnyOf(IsOk(), ExceptionIs(EX_UNSUPPORTED_OPERATION)));
    if (!status.isOk()) return;
    ASSERT_THAT(value, Ge(0));
}

/*
 * Tests the values returned by getCurrentNowMicroamps() from interface IHealth.
 */
TEST_P(HealthAidl, getCurrentNowMicroamps) {
    int32_t value;
    auto status = health->getCurrentNowMicroamps(&value);
    ASSERT_THAT(status, AnyOf(IsOk(), ExceptionIs(EX_UNSUPPORTED_OPERATION)));
    if (!status.isOk()) return;
    ASSERT_THAT(value, Not(INT32_MIN));
}

/*
 * Tests the values returned by getCurrentAverageMicroamps() from interface IHealth.
 */
TEST_P(HealthAidl, getCurrentAverageMicroamps) {
    int32_t value;
    auto status = health->getCurrentAverageMicroamps(&value);
    ASSERT_THAT(status, AnyOf(IsOk(), ExceptionIs(EX_UNSUPPORTED_OPERATION)));
    if (!status.isOk()) return;
    ASSERT_THAT(value, Not(INT32_MIN));
}

/*
 * Tests the values returned by getCapacity() from interface IHealth.
 */
TEST_P(HealthAidl, getCapacity) {
    int32_t value;
    auto status = health->getCapacity(&value);
    ASSERT_THAT(status, AnyOf(IsOk(), ExceptionIs(EX_UNSUPPORTED_OPERATION)));
    if (!status.isOk()) return;
    ASSERT_THAT(value, InClosedRange(0, 100));
}

/*
 * Tests the values returned by getEnergyCounterNwh() from interface IHealth.
 */
TEST_P(HealthAidl, getEnergyCounterNwh) {
    int64_t value;
    auto status = health->getEnergyCounterNwh(&value);
    ASSERT_THAT(status, AnyOf(IsOk(), ExceptionIs(EX_UNSUPPORTED_OPERATION)));
    if (!status.isOk()) return;
    ASSERT_THAT(value, Not(INT64_MIN));
}

/*
 * Tests the values returned by getChargeStatus() from interface IHealth.
 */
TEST_P(HealthAidl, getChargeStatus) {
    BatteryStatus value;
    auto status = health->getChargeStatus(&value);
    ASSERT_THAT(status, AnyOf(IsOk(), ExceptionIs(EX_UNSUPPORTED_OPERATION)));
    if (!status.isOk()) return;
    ASSERT_THAT(value, IsValidEnum<BatteryStatus>());
}

/*
 * Tests the values returned by getChargingPolicy() from interface IHealth.
 */
TEST_P(HealthAidl, getChargingPolicy) {
    int32_t version = 0;
    auto status = health->getInterfaceVersion(&version);
    ASSERT_TRUE(status.isOk()) << status;
    if (version < 2) {
        GTEST_SKIP() << "Support in health hal v2 for EU Ecodesign";
    }
    BatteryChargingPolicy value;
    status = health->getChargingPolicy(&value);
    ASSERT_THAT(status, AnyOf(IsOk(), ExceptionIs(EX_UNSUPPORTED_OPERATION)));
    if (!status.isOk()) return;
    ASSERT_THAT(value, IsValidEnum<BatteryChargingPolicy>());
}

/*
 * Tests that setChargingPolicy() writes the value and compared the returned
 * value by getChargingPolicy() from interface IHealth.
 */
TEST_P(HealthAidl, setChargingPolicy) {
    int32_t version = 0;
    auto status = health->getInterfaceVersion(&version);
    ASSERT_TRUE(status.isOk()) << status;
    if (version < 2) {
        GTEST_SKIP() << "Support in health hal v2 for EU Ecodesign";
    }

    BatteryChargingPolicy value;

    /* set ChargingPolicy*/
    status = health->setChargingPolicy(BatteryChargingPolicy::LONG_LIFE);
    ASSERT_THAT(status, AnyOf(IsOk(), ExceptionIs(EX_UNSUPPORTED_OPERATION)));
    if (!status.isOk()) return;

    /* get ChargingPolicy*/
    status = health->getChargingPolicy(&value);
    ASSERT_THAT(status, AnyOf(IsOk(), ExceptionIs(EX_UNSUPPORTED_OPERATION)));
    if (!status.isOk()) return;
    // the result of getChargingPolicy will be one of default(1), ADAPTIVE_AON(2)
    // ADAPTIVE_AC(3) or LONG_LIFE(4). default(1) means NOT_SUPPORT
    ASSERT_THAT(static_cast<int>(value), AnyOf(Eq(1), Eq(4)));
}

MATCHER_P(IsValidHealthData, version, "") {
    *result_listener << "value is " << arg.toString() << ".";
    if (!ExplainMatchResult(Ge(-1), arg.batteryManufacturingDateSeconds, result_listener)) {
        *result_listener << " for batteryManufacturingDateSeconds.";
        return false;
    }
    if (!ExplainMatchResult(Ge(-1), arg.batteryFirstUsageSeconds, result_listener)) {
        *result_listener << " for batteryFirstUsageSeconds.";
        return false;
    }
    if (!ExplainMatchResult(Ge(-1), arg.batteryStateOfHealth, result_listener)) {
        *result_listener << " for batteryStateOfHealth.";
        return false;
    }
    if (!ExplainMatchResult(IsValidSerialNumber(), arg.batterySerialNumber, result_listener)) {
        *result_listener << " for batterySerialNumber.";
        return false;
    }
    if (!ExplainMatchResult(IsValidEnum<BatteryPartStatus>(), arg.batteryPartStatus,
                            result_listener)) {
        *result_listener << " for batteryPartStatus.";
        return false;
    }

    return true;
}

/*
 * Tests the values returned by getBatteryHealthData() from interface IHealth.
 */
TEST_P(HealthAidl, getBatteryHealthData) {
    int32_t version = 0;
    auto status = health->getInterfaceVersion(&version);
    ASSERT_TRUE(status.isOk()) << status;
    if (version < 2) {
        GTEST_SKIP() << "Support in health hal v2 for EU Ecodesign";
    }

    BatteryHealthData value;
    status = health->getBatteryHealthData(&value);
    ASSERT_THAT(status, AnyOf(IsOk(), ExceptionIs(EX_UNSUPPORTED_OPERATION)));
    if (!status.isOk()) return;
    ASSERT_THAT(value, IsValidHealthData(version));
}

MATCHER(IsValidStorageInfo, "") {
    *result_listener << "value is " << arg.toString() << ".";
    if (!ExplainMatchResult(InClosedRange(0, 3), arg.eol, result_listener)) {
        *result_listener << " for eol.";
        return false;
    }
    if (!ExplainMatchResult(InClosedRange(0, 0x0B), arg.lifetimeA, result_listener)) {
        *result_listener << " for lifetimeA.";
        return false;
    }
    if (!ExplainMatchResult(InClosedRange(0, 0x0B), arg.lifetimeB, result_listener)) {
        *result_listener << " for lifetimeB.";
        return false;
    }
    return true;
}

/*
 * Tests the values returned by getStorageInfo() from interface IHealth.
 */
TEST_P(HealthAidl, getStorageInfo) {
    std::vector<StorageInfo> value;
    auto status = health->getStorageInfo(&value);
    ASSERT_THAT(status, AnyOf(IsOk(), ExceptionIs(EX_UNSUPPORTED_OPERATION)));
    if (!status.isOk()) return;
    ASSERT_THAT(value, Each(IsValidStorageInfo()));
}

/*
 * Tests the values returned by getDiskStats() from interface IHealth.
 */
TEST_P(HealthAidl, getDiskStats) {
    std::vector<DiskStats> value;
    auto status = health->getDiskStats(&value);
    ASSERT_THAT(status, AnyOf(IsOk(), ExceptionIs(EX_UNSUPPORTED_OPERATION)));
}

MATCHER(IsValidHealthInfo, "") {
    *result_listener << "value is " << arg.toString() << ".";
    if (!ExplainMatchResult(Each(IsValidStorageInfo()), arg.storageInfos, result_listener)) {
        *result_listener << " for storageInfos.";
        return false;
    }

    if (!ExplainMatchResult(Not(INT32_MIN), arg.batteryCurrentMicroamps, result_listener)) {
        *result_listener << " for batteryCurrentMicroamps.";
        return false;
    }

    if (!ExplainMatchResult(InClosedRange(0, 100), arg.batteryLevel, result_listener)) {
        *result_listener << " for batteryLevel.";
        return false;
    }

    if (!ExplainMatchResult(IsValidEnum<BatteryHealth>(), arg.batteryHealth, result_listener)) {
        *result_listener << " for batteryHealth.";
        return false;
    }

    if (!ExplainMatchResult(IsValidEnum<BatteryStatus>(), arg.batteryStatus, result_listener)) {
        *result_listener << " for batteryStatus.";
        return false;
    }

    if (arg.batteryPresent) {
        if (!ExplainMatchResult(Gt(0), arg.batteryChargeCounterUah, result_listener)) {
            *result_listener << " for batteryChargeCounterUah when battery is present.";
            return false;
        }
        if (!ExplainMatchResult(Not(BatteryStatus::UNKNOWN), arg.batteryStatus, result_listener)) {
            *result_listener << " for batteryStatus when battery is present.";
            return false;
        }
    }

    if (!ExplainMatchResult(IsValidEnum<BatteryCapacityLevel>(), arg.batteryCapacityLevel,
                            result_listener)) {
        *result_listener << " for batteryCapacityLevel.";
        return false;
    }
    if (!ExplainMatchResult(Ge(-1), arg.batteryChargeTimeToFullNowSeconds, result_listener)) {
        *result_listener << " for batteryChargeTimeToFullNowSeconds.";
        return false;
    }

    if (!ExplainMatchResult(
                AnyOf(Eq(0), AllOf(Gt(kFullChargeDesignCapMinUah), Lt(kFullChargeDesignCapMaxUah))),
                arg.batteryFullChargeDesignCapacityUah, result_listener)) {
        *result_listener << " for batteryFullChargeDesignCapacityUah. It should be greater than "
                            "100 mAh and less than 100,000 mAh, or 0 if unknown";
        return false;
    }

    return true;
}

/*
 * Tests the values returned by getHealthInfo() from interface IHealth.
 */
TEST_P(HealthAidl, getHealthInfo) {
    HealthInfo value;
    auto status = health->getHealthInfo(&value);
    ASSERT_THAT(status, AnyOf(IsOk(), ExceptionIs(EX_UNSUPPORTED_OPERATION)));
    if (!status.isOk()) return;
    ASSERT_THAT(value, IsValidHealthInfo());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HealthAidl);
INSTANTIATE_TEST_SUITE_P(Health, HealthAidl,
                         testing::ValuesIn(getAidlHalInstanceNames(IHealth::descriptor)),
                         PrintInstanceNameToString);

// For battery current tests, value may not be stable if the battery current has fluctuated.
// Retry in a bit more time (with the following timeout) and consider the test successful if it
// has succeed once.
static constexpr auto gBatteryTestTimeout = 1min;
static constexpr double gCurrentCompareFactor = 0.50;
class BatteryTest : public HealthAidl {};

// Tuple for all IHealth::get* API return values.
template <typename T>
struct HalResult {
    std::shared_ptr<ScopedAStatus> result = std::make_shared<ScopedAStatus>();
    T value;
};

// Needs to be called repeatedly within a period of time to ensure values are initialized.
static AssertionResult IsBatteryCurrentSignCorrect(const HalResult<BatteryStatus>& status,
                                                   const HalResult<int32_t>& current,
                                                   bool acceptZeroCurrentAsUnknown) {
    // getChargeStatus / getCurrentNow / getCurrentAverage / getHealthInfo already tested above.
    // Here, just skip if not ok.
    if (!status.result->isOk()) {
        return AssertionSuccess() << "getChargeStatus / getHealthInfo returned "
                                  << status.result->getDescription() << ", skipping";
    }

    if (!current.result->isOk()) {
        return AssertionSuccess() << "getCurrentNow / getCurrentAverage returned "
                                  << current.result->getDescription() << ", skipping";
    }

    return ::android::hardware::health::test_utils::IsBatteryCurrentSignCorrect(
            status.value, current.value, acceptZeroCurrentAsUnknown,
            [](BatteryStatus status) { return toString(status); });
}

static AssertionResult IsBatteryCurrentSimilar(const HalResult<BatteryStatus>& status,
                                               const HalResult<int32_t>& current_now,
                                               const HalResult<int32_t>& current_average) {
    if (status.result->isOk() && status.value == BatteryStatus::FULL) {
        // No reason to test on full battery because battery current load fluctuates.
        return AssertionSuccess() << "Battery is full, skipping";
    }

    // getCurrentNow / getCurrentAverage / getHealthInfo already tested above. Here, just skip if
    // not SUCCESS or value 0.
    if (!current_now.result->isOk() || current_now.value == 0) {
        return AssertionSuccess() << "getCurrentNow returned "
                                  << current_now.result->getDescription() << " with value "
                                  << current_now.value << ", skipping";
    }

    if (!current_average.result->isOk() || current_average.value == 0) {
        return AssertionSuccess() << "getCurrentAverage returned "
                                  << current_average.result->getDescription() << " with value "
                                  << current_average.value << ", skipping";
    }

    return ::android::hardware::health::test_utils::IsBatteryCurrentSimilar(
            current_now.value, current_average.value, gCurrentCompareFactor);
}

TEST_P(BatteryTest, InstantCurrentAgainstChargeStatusInHealthInfo) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<HealthInfo> health_info;
        *health_info.result = health->getHealthInfo(&health_info.value);

        return IsBatteryCurrentSignCorrect(
                {health_info.result, health_info.value.batteryStatus},
                {health_info.result, health_info.value.batteryCurrentMicroamps},
                true /* accept zero current as unknown */);
    };
    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when current_now becomes stable.";
}

TEST_P(BatteryTest, AverageCurrentAgainstChargeStatusInHealthInfo) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<HealthInfo> health_info;
        *health_info.result = health->getHealthInfo(&health_info.value);
        return IsBatteryCurrentSignCorrect(
                {health_info.result, health_info.value.batteryStatus},
                {health_info.result, health_info.value.batteryCurrentAverageMicroamps},
                true /* accept zero current as unknown */);
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when current_average becomes stable.";
}

TEST_P(BatteryTest, InstantCurrentAgainstAverageCurrentInHealthInfo) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<HealthInfo> health_info;
        *health_info.result = health->getHealthInfo(&health_info.value);
        return IsBatteryCurrentSimilar(
                {health_info.result, health_info.value.batteryStatus},
                {health_info.result, health_info.value.batteryCurrentMicroamps},
                {health_info.result, health_info.value.batteryCurrentAverageMicroamps});
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when current_now and current_average becomes "
               "stable.";
}

TEST_P(BatteryTest, InstantCurrentAgainstChargeStatusFromHal) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<BatteryStatus> status;
        *status.result = health->getChargeStatus(&status.value);
        HalResult<int32_t> current_now;
        *current_now.result = health->getCurrentNowMicroamps(&current_now.value);
        return IsBatteryCurrentSignCorrect(status, current_now,
                                           false /* accept zero current as unknown */);
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when current_now becomes stable.";
}

TEST_P(BatteryTest, AverageCurrentAgainstChargeStatusFromHal) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<BatteryStatus> status;
        *status.result = health->getChargeStatus(&status.value);
        HalResult<int32_t> current_average;
        *current_average.result = health->getCurrentAverageMicroamps(&current_average.value);
        return IsBatteryCurrentSignCorrect(status, current_average,
                                           false /* accept zero current as unknown */);
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when current_average becomes stable.";
}

TEST_P(BatteryTest, InstantCurrentAgainstAverageCurrentFromHal) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<BatteryStatus> status;
        *status.result = health->getChargeStatus(&status.value);
        HalResult<int32_t> current_now;
        *current_now.result = health->getCurrentNowMicroamps(&current_now.value);
        HalResult<int32_t> current_average;
        *current_average.result = health->getCurrentAverageMicroamps(&current_average.value);
        return IsBatteryCurrentSimilar(status, current_now, current_average);
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when current_average becomes stable.";
}

AssertionResult IsBatteryStatusCorrect(const HalResult<BatteryStatus>& status,
                                       const HalResult<HealthInfo>& health_info) {
    // getChargetStatus / getHealthInfo is already tested above. Here, just skip if not ok.
    if (!health_info.result->isOk()) {
        return AssertionSuccess() << "getHealthInfo returned "
                                  << health_info.result->getDescription() << ", skipping";
    }
    if (!status.result->isOk()) {
        return AssertionSuccess() << "getChargeStatus returned " << status.result->getDescription()
                                  << ", skipping";
    }
    return ::android::hardware::health::test_utils::IsBatteryStatusCorrect(
            status.value, health_info.value, [](BatteryStatus status) { return toString(status); });
}

TEST_P(BatteryTest, ConnectedAgainstStatusFromHal) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<BatteryStatus> status;
        *status.result = health->getChargeStatus(&status.value);
        HalResult<HealthInfo> health_info;
        *health_info.result = health->getHealthInfo(&health_info.value);
        return IsBatteryStatusCorrect(status, health_info);
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when battery_status becomes stable.";
}

TEST_P(BatteryTest, ConnectedAgainstStatusInHealthInfo) {
    auto testOnce = [&]() -> AssertionResult {
        HalResult<HealthInfo> health_info;
        *health_info.result = health->getHealthInfo(&health_info.value);
        return IsBatteryStatusCorrect({health_info.result, health_info.value.batteryStatus},
                                      health_info);
    };

    EXPECT_TRUE(SucceedOnce(gBatteryTestTimeout, testOnce))
            << "You may want to try again later when getHealthInfo becomes stable.";
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BatteryTest);
INSTANTIATE_TEST_SUITE_P(Health, BatteryTest,
                         testing::ValuesIn(getAidlHalInstanceNames(IHealth::descriptor)),
                         PrintInstanceNameToString);

}  // namespace aidl::android::hardware::health

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
