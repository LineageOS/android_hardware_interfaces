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
#pragma once

#include <android-base/logging.h>
#include <android/binder_interface_utils.h>
#include <utils/Mutex.h>

namespace android::hardware::radio::compat {

template <typename Interface, typename DefaultImplementation, bool isIndication = false>
class GuaranteedCallback {
    mutable std::mutex mCallbackGuard;
    std::shared_ptr<Interface> mCallback GUARDED_BY(mCallbackGuard);

  public:
    GuaranteedCallback<Interface, DefaultImplementation, isIndication>& operator=(
            const std::shared_ptr<Interface>& callback) {
        CHECK(callback);
        const std::lock_guard<std::mutex> lock(mCallbackGuard);
        mCallback = callback;
        return *this;
    }

    std::shared_ptr<Interface> get() {
        if (mCallback) return mCallback;
        const std::lock_guard<std::mutex> lock(mCallbackGuard);
        if (mCallback) return mCallback;

        LOG(isIndication ? WARNING : ERROR) << "Callback is not set";
        return mCallback = ndk::SharedRefBase::make<DefaultImplementation>();
    }

    operator bool() const { return mCallback != nullptr; }
};

}  // namespace android::hardware::radio::compat
