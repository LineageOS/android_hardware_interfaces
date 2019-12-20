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

#pragma once

namespace android::hardware::automotive::hidl_utils {

/**
 * Helper functor to fetch results from multi-return HIDL calls.
 * It's meant to be used in place of _hidl_cb callbacks.
 *
 * Please note extracting these return variables outside of the callback scope requires making
 * a copy of each return variable. This may be costly for frequently called HIDL methods with
 * non-negligible return object size. Please be cautious about performance when using this.
 *
 * Example usage:
 *     Result result;
 *     sp<ISomeInterface> iface;
 *     hidlObject->someMethod(arg1, arg2, hidl_utils::fill(&result, &iface)).assertOk();
 *     // use result and iface
 */
template <typename... T>
struct fill : public std::function<void(const T&...)> {
    /**
     * Create _hidl_cb functor that copies the call arguments to specified pointers.
     *
     * \param args... Targets to copy the call arguments to
     */
    fill(T*... args) : mTargets(args...) {}

    void operator()(const T&... args) { copy<0, T...>(args...); }

  private:
    std::tuple<T*...> mTargets;

    template <int Pos, typename First>
    inline void copy(const First& first) {
        *std::get<Pos>(mTargets) = first;
    }

    template <int Pos, typename First, typename... Rest>
    inline void copy(const First& first, const Rest&... rest) {
        *std::get<Pos>(mTargets) = first;
        copy<Pos + 1, Rest...>(rest...);
    }
};

}  // namespace android::hardware::automotive::hidl_utils
