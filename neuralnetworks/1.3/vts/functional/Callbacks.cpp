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

using V1_0::ErrorStatus;

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

Return<void> PreparedModelCallback::notify_1_3(ErrorStatus errorStatus,
                                               const sp<V1_3::IPreparedModel>& preparedModel) {
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

}  // namespace android::hardware::neuralnetworks::V1_3::implementation
