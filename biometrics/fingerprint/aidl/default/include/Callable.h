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

namespace aidl::android::hardware::biometrics::fingerprint {

// Interface for representing parameterless functions. Unlike std::function<void()>, this can also
// represent move-only lambdas.
class Callable {
  public:
    virtual void operator()() = 0;
    virtual ~Callable() = default;

    // Creates a heap-allocated Callable instance from any function object.
    template <typename T>
    static std::unique_ptr<Callable> from(T func);

  private:
    template <typename T>
    class AnyFuncWrapper;
};

// Private helper class for wrapping any function object into a Callable.
template <typename T>
class Callable::AnyFuncWrapper : public Callable {
  public:
    explicit AnyFuncWrapper(T func) : mFunc(std::move(func)) {}

    void operator()() override { mFunc(); }

  private:
    T mFunc;
};

template <typename T>
std::unique_ptr<Callable> Callable::from(T func) {
    return std::make_unique<AnyFuncWrapper<T>>(std::move(func));
}

}  // namespace aidl::android::hardware::biometrics::fingerprint