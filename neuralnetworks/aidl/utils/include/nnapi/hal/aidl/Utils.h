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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_H

#include "nnapi/hal/aidl/Conversions.h"

#include <aidl/android/hardware/neuralnetworks/IDevice.h>
#include <android-base/logging.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>

#include <type_traits>

namespace aidl::android::hardware::neuralnetworks::utils {

constexpr auto kDefaultPriority = Priority::MEDIUM;

constexpr std::optional<nn::Version> aidlVersionToCanonicalVersion(int aidlVersion) {
    switch (aidlVersion) {
        case 1:
            return nn::kVersionFeatureLevel5;
        case 2:
            return nn::kVersionFeatureLevel6;
        case 3:
            return nn::kVersionFeatureLevel7;
        case 4:
            return nn::kVersionFeatureLevel8;
        default:
            return std::nullopt;
    }
}

constexpr auto kVersion = aidlVersionToCanonicalVersion(IDevice::version).value();

template <typename Type>
nn::Result<void> validate(const Type& halObject) {
    const auto maybeCanonical = nn::convert(halObject);
    if (!maybeCanonical.has_value()) {
        return nn::error() << maybeCanonical.error().message;
    }
    return {};
}

template <typename Type>
bool valid(const Type& halObject) {
    const auto result = utils::validate(halObject);
    if (!result.has_value()) {
        LOG(ERROR) << result.error();
    }
    return result.has_value();
}

template <typename Type>
nn::Result<void> compliantVersion(const Type& canonical) {
    const auto version = NN_TRY(nn::validate(canonical));
    if (!nn::isCompliantVersion(version, kVersion)) {
        return NN_ERROR() << "Insufficient version: " << version << " vs required " << kVersion;
    }
    return {};
}

template <typename Type>
auto convertFromNonCanonical(const Type& nonCanonicalObject)
        -> decltype(convert(nn::convert(nonCanonicalObject).value())) {
    return convert(NN_TRY(nn::convert(nonCanonicalObject)));
}

template <typename Type>
constexpr std::underlying_type_t<Type> underlyingType(Type value) {
    return static_cast<std::underlying_type_t<Type>>(value);
}

nn::GeneralResult<Memory> clone(const Memory& memory);
nn::GeneralResult<Request> clone(const Request& request);
nn::GeneralResult<RequestMemoryPool> clone(const RequestMemoryPool& requestPool);
nn::GeneralResult<Model> clone(const Model& model);

nn::GeneralResult<void> handleTransportError(const ndk::ScopedAStatus& ret);

#define HANDLE_ASTATUS(ret)                                            \
    for (const auto status = handleTransportError(ret); !status.ok();) \
    return NN_ERROR(status.error().code) << status.error().message << ": "

#define HANDLE_STATUS_AIDL(status)                                                            \
    if (const ::android::nn::ErrorStatus canonical = ::android::nn::convert(status).value_or( \
                ::android::nn::ErrorStatus::GENERAL_FAILURE);                                 \
        canonical == ::android::nn::ErrorStatus::NONE) {                                      \
    } else                                                                                    \
        return NN_ERROR(canonical)

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_H
