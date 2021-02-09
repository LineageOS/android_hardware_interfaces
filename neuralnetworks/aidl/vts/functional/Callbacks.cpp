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

#define LOG_TAG "Callbacks"

#include "Callbacks.h"

#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <limits>

namespace aidl::android::hardware::neuralnetworks::implementation {

ndk::ScopedAStatus PreparedModelCallback::notify(
        ErrorStatus errorStatus, const std::shared_ptr<IPreparedModel>& preparedModel) {
    {
        std::lock_guard<std::mutex> hold(mMutex);
        // quick-return if object has already been notified
        if (mNotified) {
            return ndk::ScopedAStatus::ok();
        }
        // store results and mark as notified
        mErrorStatus = errorStatus;
        mPreparedModel = preparedModel;
        mNotified = true;
    }
    mCondition.notify_all();
    return ndk::ScopedAStatus::ok();
}

void PreparedModelCallback::wait() const {
    std::unique_lock<std::mutex> lock(mMutex);
    mCondition.wait(lock, [this] { return mNotified; });
}

ErrorStatus PreparedModelCallback::getStatus() const {
    wait();
    return mErrorStatus;
}

std::shared_ptr<IPreparedModel> PreparedModelCallback::getPreparedModel() const {
    wait();
    return mPreparedModel;
}

}  // namespace aidl::android::hardware::neuralnetworks::implementation
