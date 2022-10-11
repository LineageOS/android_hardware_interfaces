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

#include "RecurrentTimer.h"

#include <utils/Log.h>
#include <utils/SystemClock.h>

#include <inttypes.h>
#include <math.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

using ::android::base::ScopedLockAssertion;

RecurrentTimer::RecurrentTimer() : mThread(&RecurrentTimer::loop, this) {}

RecurrentTimer::~RecurrentTimer() {
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);
        mStopRequested = true;
    }
    mCond.notify_one();
    if (mThread.joinable()) {
        mThread.join();
    }
}

void RecurrentTimer::registerTimerCallback(int64_t intervalInNano,
                                           std::shared_ptr<RecurrentTimer::Callback> callback) {
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);

        // Aligns the nextTime to multiply of interval.
        int64_t nextTime = ceil(uptimeNanos() / intervalInNano) * intervalInNano;

        std::unique_ptr<CallbackInfo> info = std::make_unique<CallbackInfo>();
        info->callback = callback;
        info->interval = intervalInNano;
        info->nextTime = nextTime;

        auto it = mCallbacks.find(callback);
        if (it != mCallbacks.end()) {
            ALOGI("Replacing an existing timer callback with a new interval, current: %" PRId64
                  " ns, new: %" PRId64 " ns",
                  it->second->interval, intervalInNano);
            markOutdatedLocked(it->second);
        }
        mCallbacks[callback] = info.get();
        mCallbackQueue.push_back(std::move(info));
        // Insert the last element into the heap.
        std::push_heap(mCallbackQueue.begin(), mCallbackQueue.end(), CallbackInfo::cmp);
    }
    mCond.notify_one();
}

void RecurrentTimer::unregisterTimerCallback(std::shared_ptr<RecurrentTimer::Callback> callback) {
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);

        auto it = mCallbacks.find(callback);
        if (it == mCallbacks.end()) {
            ALOGE("No event found to unregister");
            return;
        }

        markOutdatedLocked(it->second);
        mCallbacks.erase(it);
    }

    mCond.notify_one();
}

void RecurrentTimer::markOutdatedLocked(RecurrentTimer::CallbackInfo* info) {
    info->outdated = true;
    info->callback = nullptr;
    // Make sure the first element is always valid.
    removeInvalidCallbackLocked();
}

void RecurrentTimer::removeInvalidCallbackLocked() {
    while (mCallbackQueue.size() != 0 && mCallbackQueue[0]->outdated) {
        std::pop_heap(mCallbackQueue.begin(), mCallbackQueue.end(), CallbackInfo::cmp);
        mCallbackQueue.pop_back();
    }
}

std::unique_ptr<RecurrentTimer::CallbackInfo> RecurrentTimer::popNextCallbackLocked() {
    std::pop_heap(mCallbackQueue.begin(), mCallbackQueue.end(), CallbackInfo::cmp);
    std::unique_ptr<CallbackInfo> info = std::move(mCallbackQueue[mCallbackQueue.size() - 1]);
    mCallbackQueue.pop_back();
    // Make sure the first element is always valid.
    removeInvalidCallbackLocked();
    return info;
}

void RecurrentTimer::loop() {
    std::unique_lock<std::mutex> uniqueLock(mLock);

    while (true) {
        // Wait until the timer exits or we have at least one recurrent callback.
        mCond.wait(uniqueLock, [this] {
            ScopedLockAssertion lockAssertion(mLock);
            return mStopRequested || mCallbackQueue.size() != 0;
        });

        int64_t interval;
        {
            ScopedLockAssertion lockAssertion(mLock);
            if (mStopRequested) {
                return;
            }
            // The first element is the nearest next event.
            int64_t nextTime = mCallbackQueue[0]->nextTime;
            int64_t now = uptimeNanos();
            if (nextTime > now) {
                interval = nextTime - now;
            } else {
                interval = 0;
            }
        }

        // Wait for the next event or the timer exits.
        if (mCond.wait_for(uniqueLock, std::chrono::nanoseconds(interval), [this] {
                ScopedLockAssertion lockAssertion(mLock);
                return mStopRequested;
            })) {
            return;
        }

        {
            ScopedLockAssertion lockAssertion(mLock);
            int64_t now = uptimeNanos();
            while (mCallbackQueue.size() > 0) {
                int64_t nextTime = mCallbackQueue[0]->nextTime;
                if (nextTime > now) {
                    break;
                }

                std::unique_ptr<CallbackInfo> info = popNextCallbackLocked();
                info->nextTime += info->interval;

                auto callback = info->callback;
                mCallbackQueue.push_back(std::move(info));
                std::push_heap(mCallbackQueue.begin(), mCallbackQueue.end(), CallbackInfo::cmp);

                (*callback)();
            }
        }
    }
}

bool RecurrentTimer::CallbackInfo::cmp(const std::unique_ptr<RecurrentTimer::CallbackInfo>& lhs,
                                       const std::unique_ptr<RecurrentTimer::CallbackInfo>& rhs) {
    return lhs->nextTime > rhs->nextTime;
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
