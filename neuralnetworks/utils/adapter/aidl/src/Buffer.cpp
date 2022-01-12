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

#include "Buffer.h"

#include <aidl/android/hardware/neuralnetworks/BnBuffer.h>
#include <aidl/android/hardware/neuralnetworks/Memory.h>
#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <nnapi/IBuffer.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/aidl/Conversions.h>

namespace aidl::android::hardware::neuralnetworks::adapter {
namespace {

template <typename Type>
auto convertInput(const Type& object) -> decltype(nn::convert(std::declval<Type>())) {
    auto result = nn::convert(object);
    if (!result.has_value()) {
        result.error().code = nn::ErrorStatus::INVALID_ARGUMENT;
    }
    return result;
}

nn::GeneralResult<std::vector<uint32_t>> inputToUnsigned(const std::vector<int32_t>& dims) {
    auto result = nn::toUnsigned(dims);
    if (!result.has_value()) {
        result.error().code = nn::ErrorStatus::INVALID_ARGUMENT;
    }
    return result;
}

nn::GeneralResult<void> copyTo(const nn::IBuffer& buffer, const Memory& dst) {
    const auto nnDst = NN_TRY(convertInput(dst));
    return buffer.copyTo(nnDst);
}

nn::GeneralResult<void> copyFrom(const nn::IBuffer& buffer, const Memory& src,
                                 const std::vector<int32_t>& dimensions) {
    const auto nnSrc = NN_TRY(convertInput(src));
    const auto nnDims = NN_TRY(inputToUnsigned(dimensions));
    return buffer.copyFrom(nnSrc, nnDims);
}

}  // namespace

Buffer::Buffer(nn::SharedBuffer buffer) : kBuffer(std::move(buffer)) {
    CHECK(kBuffer != nullptr);
}

ndk::ScopedAStatus Buffer::copyTo(const Memory& dst) {
    const auto result = adapter::copyTo(*kBuffer, dst);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Buffer::copyFrom(const Memory& src, const std::vector<int32_t>& dimensions) {
    const auto result = adapter::copyFrom(*kBuffer, src, dimensions);
    if (!result.has_value()) {
        const auto& [message, code] = result.error();
        const auto aidlCode = utils::convert(code).value_or(ErrorStatus::GENERAL_FAILURE);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                static_cast<int32_t>(aidlCode), message.c_str());
    }
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::neuralnetworks::adapter
