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

#include <type_traits>
#include <variant>

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

/**
 * Converts hidl_array<T> HIDL list to std::vector<T> AIDL list.
 *
 * To convert values, the template uses toAidl functions for a given type T, assuming it's defined.
 *
 * \param inp array to convert
 */
template <typename T, size_t N>
auto toAidl(const hidl_array<T, N>& inp) {
    std::vector<decltype(toAidl(T{}))> out(N);
    for (size_t i = 0; i < N; i++) {
        out[i] = toAidl(inp[i]);
    }
    return out;
}

/**
 * Converts T=OptionalX HIDL value to std::optional<X> AIDL value.
 *
 * To convert values, the template uses toAidl functions for a given type T.value.
 */
template <typename T>
std::optional<decltype(toAidl(T{}.value()))> toAidl(const T& opt) {
    if (opt.getDiscriminator() == T::hidl_discriminator::noinit) return std::nullopt;
    return toAidl(opt.value());
}

/**
 * Converts T=OptionalX HIDL value to std::variant<bool, X> AIDL value.
 *
 * For some reason, not every OptionalX gets generated into a std::optional<X>.
 */
template <typename T>
std::variant<bool, decltype(toAidl(T{}.value()))> toAidlVariant(const T& opt) {
    if (opt.getDiscriminator() == T::hidl_discriminator::noinit) return false;
    return toAidl(opt.value());
}

/**
 * Converts std::optional<X> AIDL value to T=OptionalX HIDL value.
 *
 * X is inferred from toAidl(T.value) declaration. Please note that toAidl(T.value) doesn't have to
 * be implemented if it's not needed for anything else than giving this hint to type system.
 *
 * To convert values, the template uses toHidl functions for a given type T, assuming it's defined.
 *
 * \param opt value to convert
 */
template <typename T>
T toHidl(const std::optional<decltype(toAidl(T{}.value()))>& opt) {
    T hidl;
    if (opt.has_value()) hidl.value(toHidl(*opt));
    return hidl;
}

/**
 * Converts U AIDL bitfield value to HIDL T bitfield value.
 *
 * \param val value to convert
 */
template <typename T, typename U>
hidl_bitfield<T> toHidlBitfield(U val) {
    return static_cast<int>(val);
}

}  // namespace android::hardware::radio::compat
