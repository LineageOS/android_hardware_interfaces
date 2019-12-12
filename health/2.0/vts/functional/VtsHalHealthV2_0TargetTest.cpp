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

#include <mutex>
#include <set>
#include <string>

#include <android-base/logging.h>
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

DEFINE_bool(force, false, "Force test healthd even when the default instance is present.");

// If GTEST_SKIP is not implemented, use our own skipping mechanism
#ifndef GTEST_SKIP
static std::mutex gSkippedTestsMutex;
static std::set<std::string> gSkippedTests;
static std::string GetCurrentTestName() {
    const auto& info = ::testing::UnitTest::GetInstance()->current_test_info();
#ifdef GTEST_REMOVE_LEGACY_TEST_CASEAPI_
    std::string test_suite = info->test_suite_name();
#else
    std::string test_suite = info->test_case_name();
#endif
    return test_suite + "." + info->name();
}

#define GTEST_SKIP()                                           \
    do {                                                       \
        std::unique_lock<std::mutex> lock(gSkippedTestsMutex); \
        gSkippedTests.insert(GetCurrentTestName());            \
        return;                                                \
    } while (0)

#define SKIP_IF_SKIPPED()                                                      \
    do {                                                                       \
        std::unique_lock<std::mutex> lock(gSkippedTestsMutex);                 \
        if (gSkippedTests.find(GetCurrentTestName()) != gSkippedTests.end()) { \
            std::cerr << "[  SKIPPED ] " << GetCurrentTestName() << std::endl; \
            return;                                                            \
        }                                                                      \
    } while (0)
#else
#define SKIP_IF_SKIPPED()
#endif

namespace android {
namespace hardware {
namespace health {
namespace V2_0 {

using V1_0::BatteryStatus;

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
    SKIP_IF_SKIPPED();
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
    SKIP_IF_SKIPPED();
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
    SKIP_IF_SKIPPED();
    EXPECT_OK(mHealth->getChargeCounter([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value > 0);
    }));
}

/*
 * Tests the values returned by getCurrentNow() from interface IHealth.
 */
TEST_P(HealthHidlTest, getCurrentNow) {
    SKIP_IF_SKIPPED();
    EXPECT_OK(mHealth->getCurrentNow([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value != INT32_MIN);
    }));
}

/*
 * Tests the values returned by getCurrentAverage() from interface IHealth.
 */
TEST_P(HealthHidlTest, getCurrentAverage) {
    SKIP_IF_SKIPPED();
    EXPECT_OK(mHealth->getCurrentAverage([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value != INT32_MIN);
    }));
}

/*
 * Tests the values returned by getCapacity() from interface IHealth.
 */
TEST_P(HealthHidlTest, getCapacity) {
    SKIP_IF_SKIPPED();
    EXPECT_OK(mHealth->getCapacity([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), 0 <= value && value <= 100);
    }));
}

/*
 * Tests the values returned by getEnergyCounter() from interface IHealth.
 */
TEST_P(HealthHidlTest, getEnergyCounter) {
    SKIP_IF_SKIPPED();
    EXPECT_OK(mHealth->getEnergyCounter([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value != INT64_MIN);
    }));
}

/*
 * Tests the values returned by getChargeStatus() from interface IHealth.
 */
TEST_P(HealthHidlTest, getChargeStatus) {
    SKIP_IF_SKIPPED();
    EXPECT_OK(mHealth->getChargeStatus([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(
            result, toString(value),
            value != BatteryStatus::UNKNOWN && verifyEnum<BatteryStatus>(value));
    }));
}

/*
 * Tests the values returned by getStorageInfo() from interface IHealth.
 */
TEST_P(HealthHidlTest, getStorageInfo) {
    SKIP_IF_SKIPPED();
    EXPECT_OK(mHealth->getStorageInfo([](auto result, auto& value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, toString(value), verifyStorageInfo(value));
    }));
}

/*
 * Tests the values returned by getDiskStats() from interface IHealth.
 */
TEST_P(HealthHidlTest, getDiskStats) {
    SKIP_IF_SKIPPED();
    EXPECT_OK(mHealth->getDiskStats([](auto result, auto& value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, toString(value), true);
    }));
}

/*
 * Tests the values returned by getHealthInfo() from interface IHealth.
 */
TEST_P(HealthHidlTest, getHealthInfo) {
    SKIP_IF_SKIPPED();
    EXPECT_OK(mHealth->getHealthInfo([](auto result, auto& value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, toString(value), verifyHealthInfo(value));
    }));
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, HealthHidlTest,
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
