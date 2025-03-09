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

#include "PendingRequestPool.h"

#include <VehicleHalTypes.h>
#include <VehicleUtils.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <unordered_set>
#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

using ::aidl::android::hardware::automotive::vehicle::StatusCode;

using ::testing::ElementsAre;
using ::testing::UnorderedElementsAre;
using ::testing::WhenSorted;

class PendingRequestPoolTest : public ::testing::Test {
  public:
    void SetUp() override { mPool = std::make_unique<PendingRequestPool>(TEST_TIMEOUT); }

    void TearDown() override {
        if (mPool != nullptr) {
            ASSERT_EQ(mPool->countPendingRequests(getTestClientId()), static_cast<size_t>(0))
                    << "at least one pending request still exists in the pool when finish";
        }
    }

    PendingRequestPool* getPool() { return mPool.get(); }

    void destroyPool() { mPool.reset(); }

    int64_t getTimeout() { return TEST_TIMEOUT; }

    const void* getTestClientId() { return reinterpret_cast<const void*>(0); }

  private:
    // Test timeout is 0.1s.
    static const int64_t TEST_TIMEOUT = 100000000;

    std::unique_ptr<PendingRequestPool> mPool;
};

TEST_F(PendingRequestPoolTest, testFinishAllRequests) {
    std::mutex lock;
    std::vector<int64_t> timeoutRequestIds;

    std::unordered_set<int64_t> requestIds;
    for (int64_t i = 0; i < 10; i++) {
        requestIds.insert(i);
    }

    auto callback = std::make_shared<PendingRequestPool::TimeoutCallbackFunc>(
            [&lock, &timeoutRequestIds](const std::unordered_set<int64_t>& requests) {
                std::scoped_lock<std::mutex> lockGuard(lock);
                for (int64_t request : requests) {
                    timeoutRequestIds.push_back(request);
                }
            });

    ASSERT_RESULT_OK(getPool()->addRequests(getTestClientId(), requestIds, callback));

    for (int64_t i = 0; i < 10; i++) {
        ASSERT_TRUE(getPool()->isRequestPending(getTestClientId(), i));
    }

    for (int64_t i = 0; i < 10; i++) {
        ASSERT_THAT(getPool()->tryFinishRequests(getTestClientId(), {i}), UnorderedElementsAre(i));
    }

    for (int64_t i = 0; i < 10; i++) {
        ASSERT_FALSE(getPool()->isRequestPending(getTestClientId(), i));
    }
}

TEST_F(PendingRequestPoolTest, testFinishHalfOfRequest) {
    int64_t timeout = getTimeout();
    std::mutex lock;
    std::vector<int64_t> timeoutRequestIds;

    std::unordered_set<int64_t> requestIds;
    for (int64_t i = 0; i < 10; i++) {
        requestIds.insert(i);
    }

    auto callback = std::make_shared<PendingRequestPool::TimeoutCallbackFunc>(
            [&lock, &timeoutRequestIds](const std::unordered_set<int64_t>& requests) {
                std::scoped_lock<std::mutex> lockGuard(lock);
                for (int64_t request : requests) {
                    timeoutRequestIds.push_back(request);
                }
            });

    ASSERT_RESULT_OK(getPool()->addRequests(getTestClientId(), requestIds, callback));

    for (int64_t i = 0; i < 10; i++) {
        ASSERT_TRUE(getPool()->isRequestPending(getTestClientId(), i));
    }

    // Finish half of the requests.
    requestIds.clear();
    for (int64_t i = 0; i < 5; i++) {
        requestIds.insert(i);
    }

    ASSERT_EQ(getPool()->tryFinishRequests(getTestClientId(), requestIds), requestIds);

    for (int64_t i = 0; i < 5; i++) {
        ASSERT_FALSE(getPool()->isRequestPending(getTestClientId(), i));
    }
    for (int64_t i = 5; i < 10; i++) {
        ASSERT_TRUE(getPool()->isRequestPending(getTestClientId(), i));
    }

    // Wait until the unfinished requests timeout. The check interval is timeout, so at max we
    // would wait an additional interval, which is 2 * timeout until the callback is called.
    std::this_thread::sleep_for(2 * std::chrono::nanoseconds(timeout));

    ASSERT_THAT(timeoutRequestIds, WhenSorted(ElementsAre(5, 6, 7, 8, 9)));
}

TEST_F(PendingRequestPoolTest, testFinishRequestTwice) {
    std::mutex lock;
    std::vector<int64_t> timeoutRequestIds;

    auto callback = std::make_shared<PendingRequestPool::TimeoutCallbackFunc>(
            [&lock, &timeoutRequestIds](const std::unordered_set<int64_t>& requests) {
                std::scoped_lock<std::mutex> lockGuard(lock);
                for (int64_t request : requests) {
                    timeoutRequestIds.push_back(request);
                }
            });

    ASSERT_RESULT_OK(getPool()->addRequests(getTestClientId(), {0}, callback));

    ASSERT_THAT(getPool()->tryFinishRequests(getTestClientId(), {0}), UnorderedElementsAre(0))
            << "failed to finish an added request";
    ASSERT_TRUE(getPool()->tryFinishRequests(getTestClientId(), {0}).empty())
            << "finish a request second time must return empty result";
}

TEST_F(PendingRequestPoolTest, testFinishRequestNonExistingId) {
    std::mutex lock;
    std::vector<int64_t> timeoutRequestIds;

    auto callback = std::make_shared<PendingRequestPool::TimeoutCallbackFunc>(
            [&lock, &timeoutRequestIds](const std::unordered_set<int64_t>& requests) {
                std::scoped_lock<std::mutex> lockGuard(lock);
                for (int64_t request : requests) {
                    timeoutRequestIds.push_back(request);
                }
            });

    ASSERT_RESULT_OK(getPool()->addRequests(getTestClientId(), {0, 1, 2}, callback));

    ASSERT_THAT(getPool()->tryFinishRequests(getTestClientId(), {0, 1, 2, 3}),
                UnorderedElementsAre(0, 1, 2))
            << "finished request IDs must not contain non-existing request ID";
    // Even though one of the request to finish does not exist, the rest of the requests should be
    // finished.
    ASSERT_EQ(getPool()->countPendingRequests(getTestClientId()), static_cast<size_t>(0))
            << "requests not being finished correctly";
}

TEST_F(PendingRequestPoolTest, testFinishAfterTimeout) {
    std::mutex lock;
    std::vector<int64_t> timeoutRequestIds;

    auto callback = std::make_shared<PendingRequestPool::TimeoutCallbackFunc>(
            [&lock, &timeoutRequestIds](const std::unordered_set<int64_t>& requests) {
                std::scoped_lock<std::mutex> lockGuard(lock);
                for (int64_t request : requests) {
                    timeoutRequestIds.push_back(request);
                }
            });

    ASSERT_RESULT_OK(getPool()->addRequests(getTestClientId(), {0}, callback));

    std::this_thread::sleep_for(2 * std::chrono::nanoseconds(getTimeout()));

    ASSERT_TRUE(getPool()->tryFinishRequests(getTestClientId(), {0}).empty())
            << "finish a request after timeout must do nothing";
}

TEST_F(PendingRequestPoolTest, testDestroyWithPendingRequests) {
    std::mutex lock;
    std::vector<int64_t> timeoutRequestIds;

    auto callback = std::make_shared<PendingRequestPool::TimeoutCallbackFunc>(
            [&lock, &timeoutRequestIds](const std::unordered_set<int64_t>& requests) {
                std::scoped_lock<std::mutex> lockGuard(lock);
                for (int64_t request : requests) {
                    timeoutRequestIds.push_back(request);
                }
            });

    ASSERT_RESULT_OK(getPool()->addRequests(getTestClientId(), {0}, callback));

    destroyPool();

    // Before the pool is destroyed, the pending requests should be notified as timeout.
    ASSERT_THAT(timeoutRequestIds, UnorderedElementsAre(0))
            << "timeout not triggered when the pool is destroyed";
}

TEST_F(PendingRequestPoolTest, testDuplicateRequestId) {
    auto callback = std::make_shared<PendingRequestPool::TimeoutCallbackFunc>(
            [](std::unordered_set<int64_t>) {});

    ASSERT_RESULT_OK(getPool()->addRequests(getTestClientId(), {0}, callback));
    ASSERT_FALSE(getPool()->addRequests(getTestClientId(), {1, 2, 0}, callback).ok())
            << "adding duplicate request IDs must fail";

    ASSERT_THAT(getPool()->tryFinishRequests(getTestClientId(), {0}), UnorderedElementsAre(0));
}

TEST_F(PendingRequestPoolTest, testSameRequestIdForDifferentClient) {
    auto callback = std::make_shared<PendingRequestPool::TimeoutCallbackFunc>(
            [](std::unordered_set<int64_t>) {});

    ASSERT_RESULT_OK(getPool()->addRequests(reinterpret_cast<const void*>(0), {0}, callback));
    ASSERT_RESULT_OK(getPool()->addRequests(reinterpret_cast<const void*>(1), {1, 2, 0}, callback));

    ASSERT_THAT(getPool()->tryFinishRequests(reinterpret_cast<const void*>(0), {0}),
                UnorderedElementsAre(0));
    ASSERT_THAT(getPool()->tryFinishRequests(reinterpret_cast<const void*>(1), {1, 2, 0}),
                UnorderedElementsAre(0, 1, 2));
}

TEST_F(PendingRequestPoolTest, testPendingRequestCountLimit) {
    auto callback = std::make_shared<PendingRequestPool::TimeoutCallbackFunc>(
            [](std::unordered_set<int64_t>) {});

    std::unordered_set<int64_t> requests;

    // MAX_PENDING_REQUEST_PER_CLIENT = 10000
    for (size_t i = 0; i < 10000; i++) {
        requests.insert(static_cast<int64_t>(i));
    }
    ASSERT_RESULT_OK(getPool()->addRequests(reinterpret_cast<const void*>(0), requests, callback));

    auto result = getPool()->addRequests(reinterpret_cast<const void*>(0),
                                         {static_cast<int64_t>(10000)}, callback);
    ASSERT_FALSE(result.ok()) << "adding more pending requests than limit must fail";
    ASSERT_EQ(result.error().code(), StatusCode::TRY_AGAIN);

    getPool()->tryFinishRequests(reinterpret_cast<const void*>(0), requests);
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
