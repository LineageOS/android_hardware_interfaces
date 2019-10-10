/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "Callbacks"

#include "1.2/Callbacks.h"

#include <android-base/logging.h>

#include <limits>

namespace android::hardware::neuralnetworks::V1_2::implementation {

using V1_0::ErrorStatus;

constexpr Timing kNoTiming = {.timeOnDevice = std::numeric_limits<uint64_t>::max(),
                              .timeInDriver = std::numeric_limits<uint64_t>::max()};

// PreparedModelCallback methods begin here

Return<void> PreparedModelCallback::notify(ErrorStatus errorStatus,
                                           const sp<V1_0::IPreparedModel>& preparedModel) {
    {
        std::lock_guard<std::mutex> hold(mMutex);

        // quick-return if object has already been notified
        if (mNotified) {
            return Void();
        }

        // store results and mark as notified
        mErrorStatus = errorStatus;
        mPreparedModel = preparedModel;
        mNotified = true;
    }

    mCondition.notify_all();
    return Void();
}

Return<void> PreparedModelCallback::notify_1_2(ErrorStatus errorStatus,
                                               const sp<V1_2::IPreparedModel>& preparedModel) {
    return notify(errorStatus, preparedModel);
}

void PreparedModelCallback::wait() const {
    std::unique_lock<std::mutex> lock(mMutex);
    mCondition.wait(lock, [this] { return mNotified; });
}

ErrorStatus PreparedModelCallback::getStatus() const {
    wait();
    return mErrorStatus;
}

sp<V1_0::IPreparedModel> PreparedModelCallback::getPreparedModel() const {
    wait();
    return mPreparedModel;
}

// ExecutionCallback methods begin here

Return<void> ExecutionCallback::notify(ErrorStatus errorStatus) {
    notifyInternal(errorStatus, {}, kNoTiming);
    return Void();
}

Return<void> ExecutionCallback::notify_1_2(ErrorStatus errorStatus,
                                           const hidl_vec<OutputShape>& outputShapes,
                                           const Timing& timing) {
    if (errorStatus == ErrorStatus::OUTPUT_INSUFFICIENT_SIZE) {
        // outputShapes must not be empty if OUTPUT_INSUFFICIENT_SIZE.
        if (outputShapes.size() == 0) {
            LOG(ERROR) << "Notified with empty output shape vector when OUTPUT_INSUFFICIENT_SIZE";
            notifyInternal(ErrorStatus::GENERAL_FAILURE, {}, kNoTiming);
            return Void();
        }
    } else if (errorStatus != ErrorStatus::NONE) {
        // outputShapes must be empty if errorStatus is neither NONE nor OUTPUT_INSUFFICIENT_SIZE.
        if (outputShapes.size() != 0) {
            LOG(ERROR) << "Notified with non-empty output shape vector when error status is "
                          "neither NONE nor OUTPUT_INSUFFICIENT_SIZE";
            notifyInternal(ErrorStatus::GENERAL_FAILURE, {}, kNoTiming);
            return Void();
        }
    }
    notifyInternal(errorStatus, outputShapes, timing);
    return Void();
}

void ExecutionCallback::wait() const {
    std::unique_lock<std::mutex> lock(mMutex);
    mCondition.wait(lock, [this] { return mNotified; });
}

ErrorStatus ExecutionCallback::getStatus() const {
    wait();
    return mErrorStatus;
}

const std::vector<OutputShape>& ExecutionCallback::getOutputShapes() const {
    wait();
    return mOutputShapes;
}

Timing ExecutionCallback::getTiming() const {
    wait();
    return mTiming;
}

void ExecutionCallback::notifyInternal(ErrorStatus errorStatus,
                                       const hidl_vec<OutputShape>& outputShapes,
                                       const Timing& timing) {
    {
        std::lock_guard<std::mutex> hold(mMutex);

        // quick-return if object has already been notified
        if (mNotified) {
            return;
        }

        mErrorStatus = errorStatus;
        mOutputShapes = outputShapes;
        mTiming = timing;
        mNotified = true;
    }
    mCondition.notify_all();
}

}  // namespace android::hardware::neuralnetworks::V1_2::implementation
