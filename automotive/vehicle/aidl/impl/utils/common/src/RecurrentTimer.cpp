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
#include <utils/Looper.h>
#include <utils/SystemClock.h>

#include <inttypes.h>
#include <math.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {

namespace {

using ::android::base::ScopedLockAssertion;

constexpr int INVALID_ID = -1;

}  // namespace

RecurrentTimer::RecurrentTimer() {
    mHandler = sp<RecurrentMessageHandler>::make(this);
    mLooper = sp<Looper>::make(/*allowNonCallbacks=*/false);
    mThread = std::thread([this] {
        Looper::setForThread(mLooper);

        while (!mStopRequested) {
            mLooper->pollOnce(/*timeoutMillis=*/-1);
        }
    });
}

RecurrentTimer::~RecurrentTimer() {
    mStopRequested = true;
    mLooper->removeMessages(mHandler);
    mLooper->wake();
    if (mThread.joinable()) {
        mThread.join();
    }
}

int RecurrentTimer::getCallbackIdLocked(std::shared_ptr<RecurrentTimer::Callback> callback) {
    const auto& it = mIdByCallback.find(callback);
    if (it != mIdByCallback.end()) {
        return it->second;
    }
    return INVALID_ID;
}

void RecurrentTimer::registerTimerCallback(int64_t intervalInNanos,
                                           std::shared_ptr<RecurrentTimer::Callback> callback) {
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);

        int callbackId = getCallbackIdLocked(callback);

        if (callbackId == INVALID_ID) {
            callbackId = mCallbackId++;
            mIdByCallback.insert({callback, callbackId});
        } else {
            ALOGI("Replacing an existing timer callback with a new interval, current: %" PRId64
                  " ns, new: %" PRId64 " ns",
                  mCallbackInfoById[callbackId]->intervalInNanos, intervalInNanos);
            mLooper->removeMessages(mHandler, callbackId);
        }

        // Aligns the nextTime to multiply of interval.
        int64_t nextTimeInNanos = ceil(uptimeNanos() / intervalInNanos) * intervalInNanos;

        std::unique_ptr<CallbackInfo> info = std::make_unique<CallbackInfo>();
        info->callback = callback;
        info->intervalInNanos = intervalInNanos;
        info->nextTimeInNanos = nextTimeInNanos;
        mCallbackInfoById.insert({callbackId, std::move(info)});

        mLooper->sendMessageAtTime(nextTimeInNanos, mHandler, Message(callbackId));
    }
}

void RecurrentTimer::unregisterTimerCallback(std::shared_ptr<RecurrentTimer::Callback> callback) {
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);

        int callbackId = getCallbackIdLocked(callback);

        if (callbackId == INVALID_ID) {
            ALOGE("No event found to unregister");
            return;
        }

        mLooper->removeMessages(mHandler, callbackId);
        mCallbackInfoById.erase(callbackId);
        mIdByCallback.erase(callback);
    }
}

void RecurrentTimer::handleMessage(const Message& message) {
    std::shared_ptr<RecurrentTimer::Callback> callback;
    {
        std::scoped_lock<std::mutex> lockGuard(mLock);

        int callbackId = message.what;

        auto it = mCallbackInfoById.find(callbackId);
        if (it == mCallbackInfoById.end()) {
            ALOGW("The event for callback ID: %d is outdated, ignore", callbackId);
            return;
        }

        CallbackInfo* callbackInfo = it->second.get();
        callback = callbackInfo->callback;
        int64_t nowNanos = uptimeNanos();
        // intervalCount is the number of interval we have to advance until we pass now.
        size_t intervalCount =
                (nowNanos - callbackInfo->nextTimeInNanos) / callbackInfo->intervalInNanos + 1;
        callbackInfo->nextTimeInNanos += intervalCount * callbackInfo->intervalInNanos;

        mLooper->sendMessageAtTime(callbackInfo->nextTimeInNanos, mHandler, Message(callbackId));
    }

    (*callback)();
}

void RecurrentMessageHandler::handleMessage(const Message& message) {
    mTimer->handleMessage(message);
}

}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android
