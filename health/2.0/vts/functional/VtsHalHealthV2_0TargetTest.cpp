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

#include <VtsHalHidlTargetTestBase.h>
#include <android-base/logging.h>
#include <android/hardware/health/2.0/IHealth.h>
#include <android/hardware/health/2.0/types.h>

using ::testing::AssertionFailure;
using ::testing::AssertionResult;
using ::testing::AssertionSuccess;
using ::testing::VtsHalHidlTargetTestBase;
using ::testing::VtsHalHidlTargetTestEnvBase;

namespace android {
namespace hardware {
namespace health {
namespace V2_0 {

using V1_0::BatteryStatus;
using V1_0::HealthInfo;

// Test environment for graphics.composer
class HealthHidlEnvironment : public VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static HealthHidlEnvironment* Instance() {
        static HealthHidlEnvironment* instance = new HealthHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IHealth>(); }

   private:
    HealthHidlEnvironment() {}

    GTEST_DISALLOW_COPY_AND_ASSIGN_(HealthHidlEnvironment);
};

class HealthHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        std::string serviceName = HealthHidlEnvironment::Instance()->getServiceName<IHealth>();
        LOG(INFO) << "get service with name:" << serviceName;
        ASSERT_FALSE(serviceName.empty());
        mHealth = ::testing::VtsHalHidlTargetTestBase::getService<IHealth>(serviceName);
        ASSERT_NE(mHealth, nullptr);
    }

    sp<IHealth> mHealth;
};

class Callback : public IHealthInfoCallback {
    using Function = std::function<void(const HealthInfo&)>;

   public:
    Callback(const Function& f) : mInternal(f) {}
    Return<void> healthInfoChanged(const HealthInfo& info) override {
        std::unique_lock<std::mutex> lock(mMutex);
        if (mInternal) mInternal(info);
        return Void();
    }
    void clear() {
        std::unique_lock<std::mutex> lock(mMutex);
        mInternal = nullptr;
    }

   private:
    std::mutex mMutex;
    Function mInternal;
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
TEST_F(HealthHidlTest, Callbacks) {
    using namespace std::chrono_literals;

    std::mutex mutex;
    std::condition_variable cv;
    bool firstCallbackInvoked = false;
    bool secondCallbackInvoked = false;

    sp<Callback> firstCallback = new Callback([&](const auto&) {
        std::unique_lock<std::mutex> lk(mutex);
        firstCallbackInvoked = true;
    });

    sp<Callback> secondCallback = new Callback([&](const auto&) {
        std::unique_lock<std::mutex> lk(mutex);
        secondCallbackInvoked = true;
        cv.notify_all();
    });

    ASSERT_ALL_OK(mHealth->registerCallback(firstCallback));
    ASSERT_ALL_OK(mHealth->registerCallback(secondCallback));

    // assert that the first callback is invoked when update is called.
    {
        std::unique_lock<std::mutex> lk(mutex);
        firstCallbackInvoked = false;
        secondCallbackInvoked = false;
    }

    ASSERT_ALL_OK(mHealth->update());

    {
        std::unique_lock<std::mutex> lk(mutex);
        EXPECT_TRUE(cv.wait_for(lk, 1s, [&] {
            return firstCallbackInvoked && secondCallbackInvoked;
        })) << "Timeout.";
        ASSERT_TRUE(firstCallbackInvoked);
        ASSERT_TRUE(secondCallbackInvoked);
    }

    ASSERT_ALL_OK(mHealth->unregisterCallback(firstCallback));

    // assert that the second callback is still invoked even though the first is unregistered.
    {
        std::unique_lock<std::mutex> lk(mutex);
        firstCallbackInvoked = false;
        secondCallbackInvoked = false;
    }

    ASSERT_ALL_OK(mHealth->update());

    {
        std::unique_lock<std::mutex> lk(mutex);
        EXPECT_TRUE(cv.wait_for(lk, 1s, [&] { return secondCallbackInvoked; })) << "Timeout.";
        ASSERT_FALSE(firstCallbackInvoked);
        ASSERT_TRUE(secondCallbackInvoked);
    }

    ASSERT_ALL_OK(mHealth->unregisterCallback(secondCallback));

    // avoid reference to lambda function that goes out of scope.
    firstCallback->clear();
    secondCallback->clear();
}

TEST_F(HealthHidlTest, UnregisterNonExistentCallback) {
    sp<Callback> callback = new Callback([](const auto&) {});
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

TEST_F(HealthHidlTest, Properties) {
    EXPECT_OK(mHealth->getChargeCounter([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value > 0);
    }));
    EXPECT_OK(mHealth->getCurrentNow([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value != INT32_MIN);
    }));
    EXPECT_OK(mHealth->getCurrentAverage([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value != INT32_MIN);
    }));
    EXPECT_OK(mHealth->getCapacity([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), 0 <= value && value <= 100);
    }));
    EXPECT_OK(mHealth->getEnergyCounter([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(result, std::to_string(value), value != INT64_MIN);
    }));
    EXPECT_OK(mHealth->getChargeStatus([](auto result, auto value) {
        EXPECT_VALID_OR_UNSUPPORTED_PROP(
            result, toString(value),
            value == BatteryStatus::CHARGING || value == BatteryStatus::DISCHARGING ||
                value == BatteryStatus::NOT_CHARGING || value == BatteryStatus::FULL);
    }));
}

}  // namespace V2_0
}  // namespace health
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    using ::android::hardware::health::V2_0::HealthHidlEnvironment;
    ::testing::AddGlobalTestEnvironment(HealthHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    HealthHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
