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

#include <android/hidl/base/1.0/IBase.h>
#include <hidl/HidlSupport.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

namespace android::hardware::neuralnetworks::utils {

template <typename Type>
nn::GeneralResult<Type> handleTransportError(const hardware::Return<Type>& ret) {
    if (ret.isDeadObject()) {
        return NN_ERROR(nn::ErrorStatus::DEAD_OBJECT)
               << "Return<>::isDeadObject returned true: " << ret.description();
    }
    if (!ret.isOk()) {
        return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
               << "Return<>::isOk returned false: " << ret.description();
    }
    return ret;
}

template <>
inline nn::GeneralResult<void> handleTransportError(const hardware::Return<void>& ret) {
    if (ret.isDeadObject()) {
        return NN_ERROR(nn::ErrorStatus::DEAD_OBJECT)
               << "Return<>::isDeadObject returned true: " << ret.description();
    }
    if (!ret.isOk()) {
        return NN_ERROR(nn::ErrorStatus::GENERAL_FAILURE)
               << "Return<>::isOk returned false: " << ret.description();
    }
    return {};
}

template <typename Type>
nn::GeneralResult<Type> makeGeneralFailure(nn::Result<Type> result, nn::ErrorStatus status) {
    if (!result.has_value()) {
        return nn::error(status) << std::move(result).error();
    }
    return std::move(result).value();
}

template <>
inline nn::GeneralResult<void> makeGeneralFailure(nn::Result<void> result, nn::ErrorStatus status) {
    if (!result.has_value()) {
        return nn::error(status) << std::move(result).error();
    }
    return {};
}

template <typename Type>
nn::ExecutionResult<Type> makeExecutionFailure(nn::Result<Type> result, nn::ErrorStatus status) {
    if (!result.has_value()) {
        return nn::error(status) << std::move(result).error();
    }
    return std::move(result).value();
}

template <>
inline nn::ExecutionResult<void> makeExecutionFailure(nn::Result<void> result,
                                                      nn::ErrorStatus status) {
    if (!result.has_value()) {
        return nn::error(status) << std::move(result).error();
    }
    return {};
}

template <typename Type>
nn::ExecutionResult<Type> makeExecutionFailure(nn::GeneralResult<Type> result) {
    if (!result.has_value()) {
        const auto [message, status] = std::move(result).error();
        return nn::error(status) << message;
    }
    return std::move(result).value();
}

template <>
inline nn::ExecutionResult<void> makeExecutionFailure(nn::GeneralResult<void> result) {
    if (!result.has_value()) {
        const auto [message, status] = std::move(result).error();
        return nn::error(status) << message;
    }
    return {};
}

}  // namespace android::hardware::neuralnetworks::utils