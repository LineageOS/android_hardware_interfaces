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

#include <android-base/logging.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>
#include <nnapi/hal/HandleError.h>

namespace aidl::android::hardware::neuralnetworks::utils {

constexpr auto kDefaultPriority = Priority::MEDIUM;
constexpr auto kVersion = nn::Version::ANDROID_S;

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
nn::GeneralResult<void> compliantVersion(const Type& canonical) {
    const auto version = NN_TRY(::android::hardware::neuralnetworks::utils::makeGeneralFailure(
            nn::validate(canonical)));
    if (version > kVersion) {
        return NN_ERROR() << "Insufficient version: " << version << " vs required " << kVersion;
    }
    return {};
}

template <typename Type>
auto convertFromNonCanonical(const Type& nonCanonicalObject)
        -> decltype(convert(nn::convert(nonCanonicalObject).value())) {
    return convert(NN_TRY(nn::convert(nonCanonicalObject)));
}

nn::GeneralResult<Memory> clone(const Memory& memory);
nn::GeneralResult<Request> clone(const Request& request);
nn::GeneralResult<RequestMemoryPool> clone(const RequestMemoryPool& requestPool);
nn::GeneralResult<Model> clone(const Model& model);

nn::GeneralResult<void> handleTransportError(const ndk::ScopedAStatus& ret);

#define HANDLE_ASTATUS(ret)                                            \
    for (const auto status = handleTransportError(ret); !status.ok();) \
    return NN_ERROR(status.error().code) << status.error().message << ": "

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_H
