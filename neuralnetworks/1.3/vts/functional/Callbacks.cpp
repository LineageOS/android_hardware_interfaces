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

#include "1.3/Callbacks.h"

#include <android-base/logging.h>

#include <limits>

namespace android::hardware::neuralnetworks::V1_3::implementation {

using V1_2::OutputShape;
using V1_2::Timing;

constexpr Timing kNoTiming = {.timeOnDevice = std::numeric_limits<uint64_t>::max(),
                              .timeInDriver = std::numeric_limits<uint64_t>::max()};

// PreparedModelCallback methods begin here

Return<void> PreparedModelCallback::notifyInternal(ErrorStatus errorStatus,
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

Return<void> PreparedModelCallback::notify(V1_0::ErrorStatus errorStatus,
                                           const sp<V1_0::IPreparedModel>& preparedModel) {
    return notifyInternal(static_cast<ErrorStatus>(errorStatus), preparedModel);
}

Return<void> PreparedModelCallback::notify_1_2(V1_0::ErrorStatus errorStatus,
                                               const sp<V1_2::IPreparedModel>& preparedModel) {
    return notifyInternal(static_cast<ErrorStatus>(errorStatus), preparedModel);
}

Return<void> PreparedModelCallback::notify_1_3(V1_3::ErrorStatus errorStatus,
                                               const sp<V1_3::IPreparedModel>& preparedModel) {
    return notifyInternal(errorStatus, preparedModel);
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

Return<void> ExecutionCallback::notify(V1_0::ErrorStatus errorStatus) {
    return notifyInternal(static_cast<ErrorStatus>(errorStatus), {}, kNoTiming);
}

Return<void> ExecutionCallback::notify_1_2(V1_0::ErrorStatus errorStatus,
                                           const hidl_vec<OutputShape>& outputShapes,
                                           const Timing& timing) {
    return notifyInternal(static_cast<ErrorStatus>(errorStatus), outputShapes, timing);
}

Return<void> ExecutionCallback::notify_1_3(V1_3::ErrorStatus errorStatus,
                                           const hidl_vec<OutputShape>& outputShapes,
                                           const Timing& timing) {
    return notifyInternal(errorStatus, outputShapes, timing);
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

Return<void> ExecutionCallback::notifyInternal(ErrorStatus errorStatus,
                                               hidl_vec<OutputShape> outputShapes, Timing timing) {
    // check results
    if (errorStatus == ErrorStatus::OUTPUT_INSUFFICIENT_SIZE) {
        // outputShapes must not be empty if OUTPUT_INSUFFICIENT_SIZE.
        if (outputShapes.size() == 0) {
            LOG(ERROR) << "Notifid with empty output shape vector when OUTPUT_INSUFFICIENT_SIZE";
            errorStatus = ErrorStatus::GENERAL_FAILURE;
            outputShapes = {};
            timing = kNoTiming;
        }
    } else if (errorStatus != ErrorStatus::NONE) {
        // outputShapes must be empty if errorStatus is neither NONE nor OUTPUT_INSUFFICIENT_SIZE.
        if (outputShapes.size() != 0) {
            LOG(ERROR) << "Notified with non-empty output shape vector when error status is "
                          "neither NONE nor OUTPUT_INSUFFICIENT_SIZE";
            errorStatus = ErrorStatus::GENERAL_FAILURE;
            outputShapes = {};
            timing = kNoTiming;
        }
    }

    // store results
    {
        std::lock_guard<std::mutex> hold(mMutex);

        // quick-return if object has already been notified
        if (mNotified) {
            return Void();
        }

        mErrorStatus = errorStatus;
        mOutputShapes = std::move(outputShapes);
        mTiming = timing;
        mNotified = true;
    }
    mCondition.notify_all();
    return Void();
}

}  // namespace android::hardware::neuralnetworks::V1_3::implementation
