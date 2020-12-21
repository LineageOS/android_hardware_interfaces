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

#include <android-base/logging.h>
#include <android-base/thread_annotations.h>

#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <type_traits>

namespace android::hardware::neuralnetworks::utils {

// This class adapts a function pointer and offers two affordances:
// 1) This class object can be used to generate a callback (via the implicit conversion operator)
//    that can be used to send the result to `CallbackValue` when called.
// 2) This class object can be used to retrieve the result of the callback with `take`.
//
// This class is thread compatible.
template <typename ReturnType, typename... ArgTypes>
class CallbackValue final {
  public:
    using FunctionType = std::add_pointer_t<ReturnType(ArgTypes...)>;
    using CallbackType = std::function<void(ArgTypes...)>;

    explicit CallbackValue(FunctionType fn);

    // Creates a callback that forwards its arguments to `mFunction` and stores the result in
    // `mReturnValue`.
    /*implicit*/ operator CallbackType();  // NOLINT(google-explicit-constructor)

    // Take the result of calling `mFunction`.
    // Precondition: mReturnValue.has_value()
    // Postcondition: !mReturnValue.has_value()
    [[nodiscard]] ReturnType take();

  private:
    std::optional<ReturnType> mReturnValue;
    FunctionType mFunction;
};

// Deduction guidelines for CallbackValue when constructed with a function pointer.
template <typename ReturnType, typename... ArgTypes>
CallbackValue(ReturnType (*)(ArgTypes...))->CallbackValue<ReturnType, ArgTypes...>;

// Thread-safe container to pass a value between threads.
template <typename Type>
class TransferValue final {
  public:
    // Put the value in `TransferValue`. If `TransferValue` already has a value, this function is a
    // no-op.
    void put(Type object) const;

    // Take the value stored in `TransferValue`. If no value is available, this function will block
    // until the value becomes available.
    // Postcondition: !mObject.has_value()
    [[nodiscard]] Type take() const;

  private:
    mutable std::mutex mMutex;
    mutable std::condition_variable mCondition;
    mutable std::optional<Type> mObject GUARDED_BY(mMutex);
};

// template implementations

template <typename ReturnType, typename... ArgTypes>
CallbackValue<ReturnType, ArgTypes...>::CallbackValue(FunctionType fn) : mFunction(fn) {}

template <typename ReturnType, typename... ArgTypes>
CallbackValue<ReturnType, ArgTypes...>::operator CallbackType() {
    return [this](ArgTypes... args) { mReturnValue = mFunction(args...); };
}

template <typename ReturnType, typename... ArgTypes>
ReturnType CallbackValue<ReturnType, ArgTypes...>::take() {
    CHECK(mReturnValue.has_value());
    std::optional<ReturnType> object;
    std::swap(object, mReturnValue);
    return std::move(object).value();
}

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
    CHECK(mObject.has_value());
    std::optional<Type> object;
    std::swap(object, mObject);
    return std::move(object).value();
}

}  // namespace android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_TRANSFER_VALUE_H
