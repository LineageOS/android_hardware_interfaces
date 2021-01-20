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

#include <unistd.h>

#include <thread>

#include <android-base/logging.h>
#include <android/hardware/health/storage/1.0/IStorage.h>
#include <gtest/gtest.h>
#include <health-storage-test/common.h>
#include <hidl/GtestPrinter.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/ServiceManagement.h>

namespace android {
namespace hardware {
namespace health {
namespace storage {
namespace V1_0 {

using namespace ::android::hardware::health::storage::test;
using ::std::literals::chrono_literals::operator""ms;

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk()) << ret.description()

class GcCallback : public IGarbageCollectCallback, public Flag {
  public:
    Return<void> onFinish(Result result) override {
        std::unique_lock<std::mutex> lock(mutex_);
        result_ = result;
        Flag::OnFinishLocked(&lock);
        return Void();
    }

    /**
     * Wait for a specific "timeout". If GC has finished, test that the result
     * is equal to the "expected" value.
     */
    template <typename R, typename P>
    void waitForResult(std::chrono::duration<R, P> timeout, Result expected) {
        std::unique_lock<std::mutex> lock(mutex_);
        ASSERT_TRUE(WaitLocked(&lock, timeout)) << "timeout after " << to_string(timeout);
        EXPECT_EQ(expected, result_);
    }

  private:
    Result result_{Result::UNKNOWN_ERROR};
};

class HealthStorageHidlTest : public ::testing::TestWithParam<std::string> {
   public:
    virtual void SetUp() override {
        fs = IStorage::getService(GetParam());

        ASSERT_NE(fs, nullptr);
        LOG(INFO) << "Service is remote " << fs->isRemote();
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
        sp<IStorage> service = fs;
        auto pingFlag = std::make_shared<Flag>();
        std::thread([service, pingFlag] {
            service->ping();
            pingFlag->OnFinish();
        })
            .detach();
        return pingFlag->Wait(timeout);
    }

    sp<IStorage> fs;
};

/**
 * Ensure garbage collection works on null callback.
 */
TEST_P(HealthStorageHidlTest, GcNullCallback) {
    auto ret = fs->garbageCollect(kDevGcTimeoutSec, nullptr);

    ASSERT_OK(ret);

    // Hold test process because HAL can be single-threaded and doing GC.
    ASSERT_TRUE(ping(kDevGcTimeout + kDevGcTolerance + kRpcTime))
            << "Service must be available after "
            << to_string(kDevGcTimeout + kDevGcTolerance + kRpcTime);
}

/**
 * Ensure garbage collection works on non-null callback.
 */
TEST_P(HealthStorageHidlTest, GcNonNullCallback) {
    sp<GcCallback> cb = new GcCallback();
    auto ret = fs->garbageCollect(kDevGcTimeoutSec, cb);
    ASSERT_OK(ret);
    cb->waitForResult(kDevGcTimeout + kDevGcTolerance + kRpcTime, Result::SUCCESS);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(HealthStorageHidlTest);
INSTANTIATE_TEST_SUITE_P(
        PerInstance, HealthStorageHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IStorage::descriptor)),
        android::hardware::PrintInstanceNameToString);

}  // namespace V1_0
}  // namespace storage
}  // namespace health
}  // namespace hardware
}  // namespace android
