/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <unistd.h>

#include <chrono>
#include <set>
#include <string>
#include <thread>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/health/storage/BnGarbageCollectCallback.h>
#include <aidl/android/hardware/health/storage/IStorage.h>
#include <android-base/logging.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>
#include <health-storage-test/common.h>

namespace aidl::android::hardware::health::storage {

using namespace ::android::hardware::health::storage::test;
using std::chrono_literals::operator""ms;

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk()) << ret.getDescription()
#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk()) << ret.getDescription()

class GcCallback : public BnGarbageCollectCallback, public Flag {
  public:
    ndk::ScopedAStatus onFinish(Result result) override {
        std::unique_lock<std::mutex> lock(mutex_);
        result_ = result;
        OnFinishLocked(&lock);
        return ndk::ScopedAStatus::ok();
    }

    /**
     * Wait for a specific "timeout". If GC has finished, test that the result
     * is equal to the "expected" value.
     */
    template <typename R, typename P>
    void WaitForResult(std::chrono::duration<R, P> timeout, Result expected) {
        std::unique_lock<std::mutex> lock(mutex_);
        ASSERT_TRUE(WaitLocked(&lock, timeout)) << "timeout after " << to_string(timeout);
        EXPECT_EQ(expected, result_);
    }

  private:
    Result result_{Result::UNKNOWN_ERROR};
};

class HealthStorageAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        std::string name = GetParam();
        ASSERT_TRUE(AServiceManager_isDeclared(name.c_str())) << name;
        ndk::SpAIBinder binder(AServiceManager_waitForService(name.c_str()));
        ASSERT_NE(binder, nullptr);
        storage_ = IStorage::fromBinder(binder);
        ASSERT_NE(storage_, nullptr);
    }

    virtual void TearDown() override {
        EXPECT_TRUE(ping(kRpcTime))
                << "Service is not responsive; expect subsequent tests to fail.";
    }

    /**
     * Ping the service and expect it to return after "timeout". Return true
     * iff the service is responsive within "timeout".
     */
    template <typename R, typename P>
    bool ping(std::chrono::duration<R, P> timeout) {
        // Ensure the service is responsive after the test.
        std::shared_ptr<IStorage> service = storage_;
        auto ping_flag = std::make_shared<Flag>();
        std::thread([service, ping_flag] {
            EXPECT_EQ(STATUS_OK, AIBinder_ping(service->asBinder().get()));
            ping_flag->OnFinish();
        }).detach();
        return ping_flag->Wait(timeout);
    }

    std::shared_ptr<IStorage> storage_;
};

/**
 * Ensure garbage collection works on null callback.
 */
TEST_P(HealthStorageAidl, GcNullCallback) {
    ASSERT_OK(storage_->garbageCollect(kDevGcTimeoutSec, nullptr));

    // Hold test process because HAL can be single-threaded and doing GC.
    ASSERT_TRUE(ping(kDevGcTimeout + kDevGcTolerance + kRpcTime))
            << "Service must be available after "
            << to_string(kDevGcTimeout + kDevGcTolerance + kRpcTime);
}

/**
 * Ensure garbage collection works on non-null callback.
 */
TEST_P(HealthStorageAidl, GcNonNullCallback) {
    std::shared_ptr<GcCallback> cb = ndk::SharedRefBase::make<GcCallback>();
    ASSERT_OK(storage_->garbageCollect(kDevGcTimeoutSec, cb));
    cb->WaitForResult(kDevGcTimeout + kDevGcTolerance + kRpcTime, Result::SUCCESS);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HealthStorageAidl);
INSTANTIATE_TEST_SUITE_P(
        HealthStorage, HealthStorageAidl,
        testing::ValuesIn(::android::getAidlHalInstanceNames(IStorage::descriptor)),
        ::android::PrintInstanceNameToString);

}  // namespace aidl::android::hardware::health::storage

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
