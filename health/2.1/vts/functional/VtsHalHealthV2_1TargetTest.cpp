/*
 * Copyright (C) 2019 The Android Open Source Project
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
#include <android/hardware/health/1.0/types.h>
#include <android/hardware/health/2.0/types.h>
#include <android/hardware/health/2.1/IHealth.h>
#include <android/hardware/health/2.1/IHealthInfoCallback.h>
#include <android/hardware/health/2.1/types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using ::android::hardware::health::V1_0::BatteryStatus;
using ::android::hardware::health::V2_0::Result;
using ::testing::AnyOf;
using ::testing::AssertionFailure;
using ::testing::AssertionResult;
using ::testing::AssertionSuccess;
using namespace std::chrono_literals;

using ::android::hardware::health::V1_0::toString;
using ::android::hardware::health::V2_0::toString;
using ::android::hardware::health::V2_1::toString;

// Return expr if it is evaluated to false.
#define TEST_AND_RETURN(expr) \
    do {                      \
        auto res = (expr);    \
        if (!res) return res; \
    } while (0)

// Return a descriptive AssertionFailure() if expr is evaluated to false.
#define TEST_AND_RETURN_FAILURE(expr)                       \
    do {                                                    \
        auto res = (expr);                                  \
        if (!res) {                                         \
            return AssertionFailure() << #expr " is false"; \
        }                                                   \
    } while (0)

namespace android {
namespace hardware {
namespace health {

namespace V2_0 {
std::ostream& operator<<(std::ostream& os, const Result& res) {
    return os << toString(res);
}
}  // namespace V2_0

namespace V2_1 {

class HealthHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        service_ = IHealth::getService(GetParam());
        ASSERT_NE(nullptr, service_.get()) << "Instance '" << GetParam() << "'' is not available.";
    }

    sp<IHealth> service_;
};

class CallbackBase {
  public:
    Return<void> healthInfoChangedInternal() {
        std::lock_guard<std::mutex> lock(mutex_);
        invoked_ = true;
        invoked_notify_.notify_all();
        return Void();
    }
    template <typename R, typename P>
    bool waitInvoke(std::chrono::duration<R, P> duration) {
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

class Callback_2_0 : public android::hardware::health::V2_0::IHealthInfoCallback,
                     public CallbackBase {
    Return<void> healthInfoChanged(const android::hardware::health::V2_0::HealthInfo&) override {
        return healthInfoChangedInternal();
    }
};

class Callback_2_1 : public android::hardware::health::V2_1::IHealthInfoCallback,
                     public CallbackBase {
    Return<void> healthInfoChanged(const android::hardware::health::V2_0::HealthInfo&) override {
        ADD_FAILURE() << "android::hardware::health::V2_1::IHealthInfoCallback::healthInfoChanged "
                      << "is called, but it shouldn't be";
        return Void();
    }
    Return<void> healthInfoChanged_2_1(const HealthInfo&) override {
        return healthInfoChangedInternal();
    }
};

template <typename T>
AssertionResult IsOk(const Return<T>& r) {
    return r.isOk() ? AssertionSuccess() : (AssertionFailure() << r.description());
}

// Both IsOk() and Result::SUCCESS
AssertionResult ResultIsSuccess(const Return<Result>& r) {
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
template <typename Callback>
AssertionResult TestCallbacks(sp<IHealth> service) {
    sp<Callback> first = new Callback();
    sp<Callback> second = new Callback();

    TEST_AND_RETURN(ResultIsSuccess(service->registerCallback(first)));
    TEST_AND_RETURN(ResultIsSuccess(service->registerCallback(second)));

    // registerCallback may or may not invoke the callback immediately, so the test needs
    // to wait for the invocation. If the implementation chooses not to invoke the callback
    // immediately, just wait for some time.
    first->waitInvoke(200ms);
    second->waitInvoke(200ms);

    // assert that the first callback is invoked when update is called.
    TEST_AND_RETURN(ResultIsSuccess(service->update()));

    TEST_AND_RETURN_FAILURE(first->waitInvoke(1s));
    TEST_AND_RETURN_FAILURE(second->waitInvoke(1s));

    TEST_AND_RETURN(ResultIsSuccess(service->unregisterCallback(first)));

    // clear any potentially pending callbacks result from wakealarm / kernel events
    // If there is none, just wait for some time.
    first->waitInvoke(200ms);
    second->waitInvoke(200ms);

    // assert that the second callback is still invoked even though the first is unregistered.
    TEST_AND_RETURN(ResultIsSuccess(service->update()));

    TEST_AND_RETURN_FAILURE(!first->waitInvoke(200ms));
    TEST_AND_RETURN_FAILURE(second->waitInvoke(1s));

    TEST_AND_RETURN(ResultIsSuccess(service->unregisterCallback(second)));
    return AssertionSuccess();
}

TEST_P(HealthHidlTest, Callbacks_2_0) {
    EXPECT_TRUE(TestCallbacks<Callback_2_0>(service_));
}

TEST_P(HealthHidlTest, Callbacks_2_1) {
    EXPECT_TRUE(TestCallbacks<Callback_2_1>(service_));
}

template <typename Callback>
AssertionResult TestUnregisterNonExistentCallback(sp<IHealth> service) {
    sp<Callback> callback = new Callback();
    auto ret = service->unregisterCallback(callback);
    TEST_AND_RETURN(IsOk(ret));
    if (static_cast<Result>(ret) != Result::NOT_FOUND) {
        return AssertionFailure()
               << "Unregistering non-existent callback should return NOT_FOUND, but returned "
               << static_cast<Result>(ret);
    }
    return AssertionSuccess();
}

TEST_P(HealthHidlTest, UnregisterNonExistentCallback_2_0) {
    EXPECT_TRUE(TestUnregisterNonExistentCallback<Callback_2_0>(service_));
}

TEST_P(HealthHidlTest, UnregisterNonExistentCallback_2_1) {
    EXPECT_TRUE(TestUnregisterNonExistentCallback<Callback_2_1>(service_));
}

template <typename T>
AssertionResult IsEnum(T value) {
    for (auto it : hidl_enum_range<T>()) {
        if (it == value) {
            return AssertionSuccess();
        }
    }

    return AssertionFailure() << static_cast<std::underlying_type_t<T>>(value) << " is not valid";
}

#define FULL_CHARGE_DESIGN_CAP_MIN ((long)100 * 1000)
#define FULL_CHARGE_DESIGN_CAP_MAX ((long)100000 * 1000)

/*
 * Tests the values returned by getHealthInfo() from interface IHealth.
 */
TEST_P(HealthHidlTest, getHealthInfo_2_1) {
    EXPECT_TRUE(IsOk(service_->getHealthInfo_2_1([](auto result, const auto& value) {
        if (result == Result::NOT_SUPPORTED) {
            return;
        }
        ASSERT_EQ(Result::SUCCESS, result);

        EXPECT_TRUE(IsEnum(value.batteryCapacityLevel)) << " BatteryCapacityLevel";
        EXPECT_GE(value.batteryChargeTimeToFullNowSeconds, 0);

        EXPECT_GE(value.batteryFullChargeDesignCapacityUah, 0)
                << "batteryFullChargeDesignCapacityUah should not be negative";

        EXPECT_GT((long)value.batteryFullChargeDesignCapacityUah, FULL_CHARGE_DESIGN_CAP_MIN)
                << "batteryFullChargeDesignCapacityUah should be greater than 100 mAh";

        EXPECT_LT((long)value.batteryFullChargeDesignCapacityUah, FULL_CHARGE_DESIGN_CAP_MAX)
                << "batteryFullChargeDesignCapacityUah should be less than 100,000 mAh";
    })));
}

TEST_P(HealthHidlTest, getHealthConfig) {
    EXPECT_TRUE(IsOk(service_->getHealthConfig([](auto result, const auto&) {
        EXPECT_THAT(result, AnyOf(Result::SUCCESS, Result::NOT_SUPPORTED));
    })));
}

TEST_P(HealthHidlTest, shouldKeepScreenOn) {
    EXPECT_TRUE(IsOk(service_->shouldKeepScreenOn([](auto result, const auto&) {
        EXPECT_THAT(result, AnyOf(Result::SUCCESS, Result::NOT_SUPPORTED));
    })));
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, HealthHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IHealth::descriptor)),
        android::hardware::PrintInstanceNameToString);

}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
