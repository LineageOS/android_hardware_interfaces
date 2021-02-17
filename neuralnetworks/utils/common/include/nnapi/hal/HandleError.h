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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_HANDLE_ERROR_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_HANDLE_ERROR_H

#include <android/hidl/base/1.0/IBase.h>
#include <hidl/HidlSupport.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <type_traits>

namespace android::hardware::neuralnetworks::utils {

template <typename Type>
nn::GeneralResult<Type> handleTransportError(const hardware::Return<Type>& ret) {
    if (ret.isDeadObject()) {
        return nn::error(nn::ErrorStatus::DEAD_OBJECT)
               << "Return<>::isDeadObject returned true: " << ret.description();
    }
    if (!ret.isOk()) {
        return nn::error(nn::ErrorStatus::GENERAL_FAILURE)
               << "Return<>::isOk returned false: " << ret.description();
    }
    if constexpr (!std::is_same_v<Type, void>) {
        return static_cast<Type>(ret);
    } else {
        return {};
    }
}

#define HANDLE_TRANSPORT_FAILURE(ret)                                                        \
    ({                                                                                       \
        auto result = ::android::hardware::neuralnetworks::utils::handleTransportError(ret); \
        if (!result.has_value()) {                                                           \
            return NN_ERROR(result.error().code) << result.error().message;                  \
        }                                                                                    \
        std::move(result).value();                                                           \
    })

template <typename Type>
nn::GeneralResult<Type> makeGeneralFailure(
        nn::Result<Type> result, nn::ErrorStatus status = nn::ErrorStatus::GENERAL_FAILURE) {
    if (!result.has_value()) {
        return nn::error(status) << std::move(result).error();
    }
    if constexpr (!std::is_same_v<Type, void>) {
        return std::move(result).value();
    } else {
        return {};
    }
}

template <typename Type>
nn::ExecutionResult<Type> makeExecutionFailure(nn::GeneralResult<Type> result) {
    if (!result.has_value()) {
        const auto [message, status] = std::move(result).error();
        return nn::error(status) << message;
    }
    if constexpr (!std::is_same_v<Type, void>) {
        return std::move(result).value();
    } else {
        return {};
    }
}

template <typename Type>
nn::ExecutionResult<Type> makeExecutionFailure(
        nn::Result<Type> result, nn::ErrorStatus status = nn::ErrorStatus::GENERAL_FAILURE) {
    return makeExecutionFailure(makeGeneralFailure(result, status));
}

#define HANDLE_HAL_STATUS(status)                                       \
    if (const auto canonical = ::android::nn::convert(status).value_or( \
                ::android::nn::ErrorStatus::GENERAL_FAILURE);           \
        canonical == ::android::nn::ErrorStatus::NONE) {                \
    } else                                                              \
        return NN_ERROR(canonical)

}  // namespace android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_HANDLE_ERROR_H
