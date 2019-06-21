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

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/health/storage/1.0/IStorage.h>
#include <hidl/HidlTransportSupport.h>
#include <unistd.h>
#include <thread>

namespace android {
namespace hardware {
namespace health {
namespace storage {
namespace V1_0 {

using ::std::literals::chrono_literals::operator""ms;

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk()) << ret.description()

// Dev GC timeout. This is the timeout used by vold.
const uint64_t kDevGcTimeoutSec = 120;
const std::chrono::seconds kDevGcTimeout{kDevGcTimeoutSec};
// Dev GC timeout tolerance. The HAL may not immediately return after the
// timeout, so include an acceptable tolerance.
const std::chrono::seconds kDevGcTolerance{3};
// Time accounted for RPC calls.
const std::chrono::milliseconds kRpcTime{1000};

template <typename R>
std::string toString(std::chrono::duration<R, std::milli> time) {
    return std::to_string(time.count()) + "ms";
}

/** An atomic boolean flag that indicates whether a task has finished. */
class Flag {
   public:
    void onFinish() {
        std::unique_lock<std::mutex> lock(mMutex);
        onFinishLocked(&lock);
    }
    template <typename R, typename P>
    bool wait(std::chrono::duration<R, P> duration) {
        std::unique_lock<std::mutex> lock(mMutex);
        return waitLocked(&lock, duration);
    }

   protected:
    /** Will unlock. */
    void onFinishLocked(std::unique_lock<std::mutex>* lock) {
        mFinished = true;
        lock->unlock();
        mCv.notify_all();
    }
    template <typename R, typename P>
    bool waitLocked(std::unique_lock<std::mutex>* lock, std::chrono::duration<R, P> duration) {
        mCv.wait_for(*lock, duration, [this] { return mFinished; });
        return mFinished;
    }

    bool mFinished{false};
    std::mutex mMutex;
    std::condition_variable mCv;
};

class GcCallback : public IGarbageCollectCallback, public Flag {
   public:
    Return<void> onFinish(Result result) override {
        std::unique_lock<std::mutex> lock(mMutex);
        mResult = result;
        Flag::onFinishLocked(&lock);
        return Void();
    }

    /**
     * Wait for a specific "timeout". If GC has finished, test that the result
     * is equal to the "expected" value.
     */
    template <typename R, typename P>
    void waitForResult(std::chrono::duration<R, P> timeout, Result expected) {
        std::unique_lock<std::mutex> lock(mMutex);
        ASSERT_TRUE(waitLocked(&lock, timeout)) << "timeout after " << toString(timeout);
        EXPECT_EQ(expected, mResult);
    }

   private:
    Result mResult{Result::UNKNOWN_ERROR};
};

/** Test environment for Health Storage HIDL HAL. */
class HealthStorageHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    /** get the test environment singleton */
    static HealthStorageHidlEnvironment* Instance() {
        static HealthStorageHidlEnvironment* instance = new HealthStorageHidlEnvironment();
        return instance;
    }
    virtual void registerTestServices() override { registerTestService<IStorage>(); }

   private:
    HealthStorageHidlEnvironment() {}
};

class HealthStorageHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        fs = ::testing::VtsHalHidlTargetTestBase::getService<IStorage>(
            HealthStorageHidlEnvironment::Instance()->getServiceName<IStorage>());

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
            pingFlag->onFinish();
        })
            .detach();
        return pingFlag->wait(timeout);
    }

    sp<IStorage> fs;
};

/**
 * Ensure garbage collection works on null callback.
 */
TEST_F(HealthStorageHidlTest, GcNullCallback) {
    auto ret = fs->garbageCollect(kDevGcTimeoutSec, nullptr);

    ASSERT_OK(ret);

    // Hold test process because HAL can be single-threaded and doing GC.
    ASSERT_TRUE(ping(kDevGcTimeout + kDevGcTolerance + kRpcTime))
            << "Service must be available after "
            << toString(kDevGcTimeout + kDevGcTolerance + kRpcTime);
}

/**
 * Ensure garbage collection works on non-null callback.
 */
TEST_F(HealthStorageHidlTest, GcNonNullCallback) {
    sp<GcCallback> cb = new GcCallback();
    auto ret = fs->garbageCollect(kDevGcTimeoutSec, cb);
    ASSERT_OK(ret);
    cb->waitForResult(kDevGcTimeout + kDevGcTolerance + kRpcTime, Result::SUCCESS);
}

}  // namespace V1_0
}  // namespace storage
}  // namespace health
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    using ::android::hardware::configureRpcThreadpool;
    using ::android::hardware::health::storage::V1_0::HealthStorageHidlEnvironment;

    configureRpcThreadpool(1, false /* callerWillJoin*/);
    ::testing::AddGlobalTestEnvironment(HealthStorageHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    HealthStorageHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
