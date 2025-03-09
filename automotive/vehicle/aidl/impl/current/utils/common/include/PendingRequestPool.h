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

#ifndef android_hardware_automotive_vehicle_aidl_impl_vhal_include_PendingRequestPool_H_
#define android_hardware_automotive_vehicle_aidl_impl_vhal_include_PendingRequestPool_H_

#include <VehicleUtils.h>
#include <android-base/result.h>
#include <android-base/thread_annotations.h>

#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

// A thread-safe pending request pool that tracks whether each request has timed-out.
class PendingRequestPool final {
  public:
    using TimeoutCallbackFunc = std::function<void(const std::unordered_set<int64_t>&)>;

    explicit PendingRequestPool(int64_t timeoutInNano);

    ~PendingRequestPool();

    // Adds a list of requests to the request pool.
    // The clientId is the key for all the requests. It could be a number or an address to a data
    // structure that represents a client. The caller must maintain this data structure.
    // All the request IDs must be unique for one client, if any of the requestIds is duplicate with
    // any pending request IDs for the client, this function returns error and no requests would be
    // added. Otherwise, they would be added to the request pool.
    // The callback would be called if requests are not finished within {@code mTimeoutInNano}
    // seconds.
    VhalResult<void> addRequests(const void* clientId,
                                 const std::unordered_set<int64_t>& requestIds,
                                 std::shared_ptr<const TimeoutCallbackFunc> callback);

    // Checks whether the request is currently pending.
    bool isRequestPending(const void* clientId, int64_t requestId) const;

    // Tries to mark the requests as finished and remove them from the pool if the request is
    // currently pending. Returns the list of request that is pending and has been finished
    // successfully. This function would try to finish any valid requestIds even though some of the
    // requestIds are not valid.
    std::unordered_set<int64_t> tryFinishRequests(const void* clientId,
                                                  const std::unordered_set<int64_t>& requestIds);

    // Returns how many pending requests in the pool, for testing purpose.
    size_t countPendingRequests(const void* clientId) const;

    size_t countPendingRequests() const;

  private:
    // The maximum number of pending requests allowed per client. If exceeds this number, adding
    // more requests would fail. This is to prevent spamming from client.
    static constexpr size_t MAX_PENDING_REQUEST_PER_CLIENT = 10000;

    struct PendingRequest {
        std::unordered_set<int64_t> requestIds;
        int64_t timeoutTimestamp;
        std::shared_ptr<const TimeoutCallbackFunc> callback;
    };

    int64_t mTimeoutInNano;
    mutable std::mutex mLock;
    std::unordered_map<const void*, std::list<PendingRequest>> mPendingRequestsByClient
            GUARDED_BY(mLock);
    std::thread mThread;
    bool mThreadStop = false;
    std::condition_variable mCv;
    std::mutex mCvLock;

    bool isRequestPendingLocked(const void* clientId, int64_t requestId) const REQUIRES(mLock);

    // Checks whether the requests in the pool has timed-out, run periodically in a separate thread.
    void checkTimeout();
};

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_aidl_impl_vhal_include_PendingRequestPool_H_
