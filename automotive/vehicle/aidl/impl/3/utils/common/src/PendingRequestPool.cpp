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

#include <utils/Log.h>
#include <utils/SystemClock.h>

#include <vector>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace {

using ::aidl::android::hardware::automotive::vehicle::StatusCode;
using ::android::base::Result;

// At least check every 1s.
constexpr int64_t CHECK_TIME_IN_NANO = 1'000'000'000;

}  // namespace

PendingRequestPool::PendingRequestPool(int64_t timeoutInNano) : mTimeoutInNano(timeoutInNano) {
    mThread = std::thread([this] {
        // [this] must be alive within this thread because destructor would wait for this thread
        // to exit.
        int64_t sleepTime = std::min(mTimeoutInNano, static_cast<int64_t>(CHECK_TIME_IN_NANO));
        std::unique_lock<std::mutex> lk(mCvLock);
        while (!mCv.wait_for(lk, std::chrono::nanoseconds(sleepTime),
                             [this] { return mThreadStop; })) {
            checkTimeout();
        }
    });
}

PendingRequestPool::~PendingRequestPool() {
    {
        // Even if the shared variable is atomic, it must be modified under the
        // mutex in order to correctly publish the modification to the waiting
        // thread.
        std::unique_lock<std::mutex> lk(mCvLock);
        mThreadStop = true;
    }
    mCv.notify_all();
    if (mThread.joinable()) {
        mThread.join();
    }

    // If this pool is being destructed, send out all pending requests as timeout.
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);

        for (auto& [_, pendingRequests] : mPendingRequestsByClient) {
            for (const auto& request : pendingRequests) {
                (*request.callback)(request.requestIds);
            }
        }
        mPendingRequestsByClient.clear();
    }
}

VhalResult<void> PendingRequestPool::addRequests(
        const void* clientId, const std::unordered_set<int64_t>& requestIds,
        std::shared_ptr<const TimeoutCallbackFunc> callback) {
    std::scoped_lock<std::mutex> lockGuard(mLock);
    std::list<PendingRequest>* pendingRequests;
    size_t pendingRequestCount = 0;
    if (mPendingRequestsByClient.find(clientId) != mPendingRequestsByClient.end()) {
        pendingRequests = &mPendingRequestsByClient[clientId];
        for (const auto& pendingRequest : *pendingRequests) {
            const auto& pendingRequestIds = pendingRequest.requestIds;
            for (int64_t requestId : requestIds) {
                if (pendingRequestIds.find(requestId) != pendingRequestIds.end()) {
                    return StatusError(StatusCode::INVALID_ARG)
                           << "duplicate request ID: " << requestId;
                }
            }
            pendingRequestCount += pendingRequestIds.size();
        }
    } else {
        // Create a new empty list for this client.
        pendingRequests = &mPendingRequestsByClient[clientId];
    }

    if (requestIds.size() > MAX_PENDING_REQUEST_PER_CLIENT - pendingRequestCount) {
        return StatusError(StatusCode::TRY_AGAIN) << "too many pending requests";
    }

    int64_t currentTime = elapsedRealtimeNano();
    int64_t timeoutTimestamp = currentTime + mTimeoutInNano;

    pendingRequests->push_back({
            .requestIds = std::unordered_set<int64_t>(requestIds.begin(), requestIds.end()),
            .timeoutTimestamp = timeoutTimestamp,
            .callback = callback,
    });

    return {};
}

bool PendingRequestPool::isRequestPending(const void* clientId, int64_t requestId) const {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    return isRequestPendingLocked(clientId, requestId);
}

size_t PendingRequestPool::countPendingRequests() const {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    size_t count = 0;
    for (const auto& [clientId, requests] : mPendingRequestsByClient) {
        for (const auto& request : requests) {
            count += request.requestIds.size();
        }
    }
    return count;
}

size_t PendingRequestPool::countPendingRequests(const void* clientId) const {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    auto it = mPendingRequestsByClient.find(clientId);
    if (it == mPendingRequestsByClient.end()) {
        return 0;
    }

    size_t count = 0;
    for (const auto& pendingRequest : it->second) {
        count += pendingRequest.requestIds.size();
    }

    return count;
}

bool PendingRequestPool::isRequestPendingLocked(const void* clientId, int64_t requestId) const {
    auto it = mPendingRequestsByClient.find(clientId);
    if (it == mPendingRequestsByClient.end()) {
        return false;
    }
    for (const auto& pendingRequest : it->second) {
        const auto& requestIds = pendingRequest.requestIds;
        if (requestIds.find(requestId) != requestIds.end()) {
            return true;
        }
    }
    return false;
}

void PendingRequestPool::checkTimeout() {
    std::vector<PendingRequest> timeoutRequests;
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);

        int64_t currentTime = elapsedRealtimeNano();

        std::vector<const void*> clientsWithEmptyRequests;

        for (auto& [clientId, pendingRequests] : mPendingRequestsByClient) {
            auto it = pendingRequests.begin();
            while (it != pendingRequests.end()) {
                if (it->timeoutTimestamp >= currentTime) {
                    break;
                }
                timeoutRequests.push_back(std::move(*it));
                it = pendingRequests.erase(it);
            }

            if (pendingRequests.empty()) {
                clientsWithEmptyRequests.push_back(clientId);
            }
        }

        for (const void* clientId : clientsWithEmptyRequests) {
            mPendingRequestsByClient.erase(clientId);
        }
    }

    // Call the callback outside the lock.
    for (const auto& request : timeoutRequests) {
        (*request.callback)(request.requestIds);
    }
}

std::unordered_set<int64_t> PendingRequestPool::tryFinishRequests(
        const void* clientId, const std::unordered_set<int64_t>& requestIds) {
    std::scoped_lock<std::mutex> lockGuard(mLock);

    std::unordered_set<int64_t> foundIds;

    if (mPendingRequestsByClient.find(clientId) == mPendingRequestsByClient.end()) {
        return foundIds;
    }

    auto& pendingRequests = mPendingRequestsByClient[clientId];
    auto it = pendingRequests.begin();
    while (it != pendingRequests.end()) {
        auto& pendingRequestIds = it->requestIds;
        for (int64_t requestId : requestIds) {
            auto idIt = pendingRequestIds.find(requestId);
            if (idIt == pendingRequestIds.end()) {
                continue;
            }
            pendingRequestIds.erase(idIt);
            foundIds.insert(requestId);
        }
        if (pendingRequestIds.empty()) {
            it = pendingRequests.erase(it);
            continue;
        }
        it++;
    }

    return foundIds;
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
