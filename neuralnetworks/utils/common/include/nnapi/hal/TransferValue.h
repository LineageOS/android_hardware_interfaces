/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_TRANSFER_VALUE_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_TRANSFER_VALUE_H

#include <android-base/thread_annotations.h>

#include <condition_variable>
#include <mutex>
#include <optional>

namespace android::hardware::neuralnetworks::utils {

// This class is thread safe.
template <typename Type>
class TransferValue final {
  public:
    void put(Type object) const;
    [[nodiscard]] Type take() const;

  private:
    mutable std::mutex mMutex;
    mutable std::condition_variable mCondition;
    mutable std::optional<Type> mObject GUARDED_BY(mMutex);
};

// template implementation

template <typename Type>
void TransferValue<Type>::put(Type object) const {
    {
        std::lock_guard guard(mMutex);
        // Immediately return if value already exists.
        if (mObject.has_value()) return;
        mObject.emplace(std::move(object));
    }
    mCondition.notify_all();
}

template <typename Type>
Type TransferValue<Type>::take() const {
    std::unique_lock lock(mMutex);
    base::ScopedLockAssertion lockAssertion(mMutex);
    mCondition.wait(lock, [this]() REQUIRES(mMutex) { return mObject.has_value(); });
    std::optional<Type> object;
    std::swap(object, mObject);
    return std::move(object).value();
}

}  // namespace android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_TRANSFER_VALUE_H
