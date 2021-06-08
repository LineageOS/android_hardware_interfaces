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

#define LOG_TAG "GeneratorHub"

#include <log/log.h>

#include "GeneratorHub.h"

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {

namespace impl {

GeneratorHub::GeneratorHub(const OnHalEvent& onHalEvent)
    : mOnHalEvent(onHalEvent), mThread(&GeneratorHub::run, this) {}

GeneratorHub::~GeneratorHub() {
    mShuttingDownFlag.store(true);
    mCond.notify_all();
    if (mThread.joinable()) {
        mThread.join();
    }
}

void GeneratorHub::registerGenerator(int32_t cookie, FakeValueGeneratorPtr generator) {
    {
        std::lock_guard<std::mutex> g(mLock);
        // Register only if the generator can produce event
        if (generator->hasNext()) {
            // Push the next event if it is a new generator
            if (mGenerators.find(cookie) == mGenerators.end()) {
                ALOGI("%s: Registering new generator, cookie: %d", __func__, cookie);
                mEventQueue.push({cookie, generator->nextEvent()});
            }
            mGenerators[cookie] = std::move(generator);
            ALOGI("%s: Registered generator, cookie: %d", __func__, cookie);
        }
    }
    mCond.notify_one();
}

void GeneratorHub::unregisterGenerator(int32_t cookie) {
    {
        std::lock_guard<std::mutex> g(mLock);
        mGenerators.erase(cookie);
    }
    mCond.notify_one();
    ALOGI("%s: Unregistered generator, cookie: %d", __func__, cookie);
}

void GeneratorHub::run() {
    while (!mShuttingDownFlag.load()) {
        std::unique_lock<std::mutex> g(mLock);
        // Pop events whose generator does not exist (may be already unregistered)
        while (!mEventQueue.empty()
               && mGenerators.find(mEventQueue.top().cookie) == mGenerators.end()) {
             mEventQueue.pop();
        }
        // Wait until event queue is not empty or shutting down flag is set
        mCond.wait(g, [this] { return !mEventQueue.empty() || mShuttingDownFlag.load(); });
        if (mShuttingDownFlag.load()) {
            break;
        }

        const VhalEvent& curEvent = mEventQueue.top();

        TimePoint eventTime(Nanos(curEvent.val.timestamp));
        // Wait until the soonest event happen
        if (mCond.wait_until(g, eventTime) != std::cv_status::timeout) {
        // It is possible that a new generator is registered and produced a sooner event, or current
        // generator is unregistered, in this case the thread will re-evaluate the soonest event
            ALOGI("Something happened while waiting");
            continue;
        }
        // Now it's time to handle current event.
        mOnHalEvent(curEvent.val);
        // Update queue by popping current event and producing next event from the same generator
        int32_t cookie = curEvent.cookie;
        mEventQueue.pop();
        if (hasNext(cookie)) {
            mEventQueue.push({cookie, mGenerators[cookie]->nextEvent()});
        } else {
            ALOGI("%s: Generator ended, unregister it, cookie: %d", __func__, cookie);
            mGenerators.erase(cookie);
        }
    }
}

bool GeneratorHub::hasNext(int32_t cookie) {
    return mGenerators.find(cookie) != mGenerators.end() && mGenerators[cookie]->hasNext();
}

}  // namespace impl

}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
