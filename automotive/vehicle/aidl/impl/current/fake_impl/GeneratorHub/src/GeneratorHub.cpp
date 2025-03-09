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

#define LOG_TAG "GeneratorHub"

#include "GeneratorHub.h"

#include <utils/Log.h>
#include <utils/SystemClock.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace fake {

using ::android::base::ScopedLockAssertion;

GeneratorHub::GeneratorHub(OnHalEvent&& onHalEvent) : mOnHalEvent(onHalEvent) {
    mThread = std::thread(&GeneratorHub::run, this);
}

GeneratorHub::~GeneratorHub() {
    {
        // Even if the shared variable is atomic, it must be modified under the
        // mutex in order to correctly publish the modification to the waiting
        // thread.
        std::unique_lock<std::mutex> lock(mGeneratorsLock);
        mShuttingDownFlag.store(true);
    }
    mCond.notify_all();
    if (mThread.joinable()) {
        mThread.join();
    }
}

void GeneratorHub::registerGenerator(int32_t id, std::unique_ptr<FakeValueGenerator> generator) {
    {
        std::scoped_lock<std::mutex> lockGuard(mGeneratorsLock);
        auto maybeNextEvent = generator->nextEvent();
        // Register only if the generator can produce at least one event.
        if (maybeNextEvent.has_value()) {
            // Push the next event if it is a new generator
            if (mGenerators.find(id) == mGenerators.end()) {
                ALOGI("%s: Registering new generator, id: %d", __func__, id);
                mEventQueue.push({id, maybeNextEvent.value()});
            }
            mGenerators[id] = std::move(generator);
            ALOGI("%s: Registered generator, id: %d", __func__, id);
        }
    }
    mCond.notify_one();
}

bool GeneratorHub::unregisterGenerator(int32_t id) {
    bool removed;
    {
        std::scoped_lock<std::mutex> lockGuard(mGeneratorsLock);
        removed = mGenerators.erase(id);
    }
    mCond.notify_one();
    ALOGI("%s: Unregistered generator, id: %d", __func__, id);
    return removed;
}

void GeneratorHub::run() {
    while (!mShuttingDownFlag.load()) {
        std::unique_lock<std::mutex> lock(mGeneratorsLock);
        ScopedLockAssertion lock_assertion(mGeneratorsLock);
        // Pop events whose generator does not exist (may be already unregistered)
        while (!mEventQueue.empty() &&
               mGenerators.find(mEventQueue.top().generatorId) == mGenerators.end()) {
            mEventQueue.pop();
        }
        // Wait until event queue is not empty or shutting down flag is set.
        // This would unlock mGeneratorsLock and reacquire later.
        mCond.wait(lock, [this] { return !mEventQueue.empty() || mShuttingDownFlag.load(); });
        if (mShuttingDownFlag.load()) {
            break;
        }

        const VhalEvent& curEvent = mEventQueue.top();
        long currentTime = elapsedRealtimeNano();
        long waitTime =
                curEvent.val.timestamp > currentTime ? curEvent.val.timestamp - currentTime : 0;
        if (waitTime != 0) {
            // Wait until the soonest event happen
            if (mCond.wait_for(lock, std::chrono::nanoseconds(waitTime)) !=
                std::cv_status::timeout) {
                // It is possible that a new generator is registered and produced a sooner event, or
                // current generator is unregistered, in this case the thread will re-evaluate the
                // soonest event
                ALOGI("Something happened while waiting");
                continue;
            }
        }
        // Now it's time to handle current event.
        mOnHalEvent(curEvent.val);
        // Update queue by popping current event and producing next event from the same generator
        int32_t id = curEvent.generatorId;
        mEventQueue.pop();
        if (mGenerators.find(id) != mGenerators.end()) {
            auto maybeNextEvent = mGenerators[id]->nextEvent();
            if (maybeNextEvent.has_value()) {
                mEventQueue.push({id, maybeNextEvent.value()});
                continue;
            }
        }

        ALOGI("%s: Generator ended, unregister it, id: %d", __func__, id);
        mGenerators.erase(id);
    }
}

}  // namespace fake
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
