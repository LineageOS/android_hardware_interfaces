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

#include <hidl/HidlSupport.h>

namespace android::hardware::radio::compat {

/**
 * Converts hidl_vec<T> HIDL list to std::vector<T> AIDL list.
 *
 * To convert values, the template uses toAidl functions for a given type T, assuming it's defined.
 *
 * \param inp vector to convert
 */
template <typename T>
auto toAidl(const hidl_vec<T>& inp) {
    std::vector<decltype(toAidl(T{}))> out(inp.size());
    for (size_t i = 0; i < inp.size(); i++) {
        out[i] = toAidl(inp[i]);
    }
    return out;
}

/**
 * Converts std::vector<T> AIDL list to hidl_vec<T> HIDL list.
 *
 * To convert values, the template uses toHidl functions for a given type T, assuming it's defined.
 *
 * \param inp vector to convert
 */
template <typename T>
auto toHidl(const std::vector<T>& inp) {
    hidl_vec<decltype(toHidl(T{}))> out(inp.size());
    for (size_t i = 0; i < inp.size(); i++) {
        out[i] = toHidl(inp[i]);
    }
    return out;
}

}  // namespace android::hardware::radio::compat
