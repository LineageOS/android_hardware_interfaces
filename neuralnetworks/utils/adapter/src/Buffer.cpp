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

#include "Buffer.h"

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.3/IBuffer.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/IBuffer.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.3/Utils.h>
#include <memory>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::adapter {
namespace {

template <typename Type>
auto convertInput(const Type& object) -> decltype(nn::convert(std::declval<Type>())) {
    auto result = nn::convert(object);
    if (!result.has_value()) {
        result.error().code = nn::ErrorStatus::INVALID_ARGUMENT;
    }
    return result;
}

nn::GeneralResult<void> copyTo(const nn::SharedBuffer& buffer, const hidl_memory& dst) {
    const auto memory = NN_TRY(convertInput(dst));
    NN_TRY(buffer->copyTo(memory));
    return {};
}

nn::GeneralResult<void> copyFrom(const nn::SharedBuffer& buffer, const hidl_memory& src,
                                 const hidl_vec<uint32_t>& dimensions) {
    const auto memory = NN_TRY(convertInput(src));
    NN_TRY(buffer->copyFrom(memory, dimensions));
    return {};
}

}  // namespace

Buffer::Buffer(nn::SharedBuffer buffer) : kBuffer(std::move(buffer)) {
    CHECK(kBuffer != nullptr);
}

Return<V1_3::ErrorStatus> Buffer::copyTo(const hidl_memory& dst) {
    auto result = adapter::copyTo(kBuffer, dst);
    if (!result.has_value()) {
        const auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::Buffer::copyTo failed with " << code << ": " << message;
        return V1_3::utils::convert(code).value();
    }
    return V1_3::ErrorStatus::NONE;
}

Return<V1_3::ErrorStatus> Buffer::copyFrom(const hidl_memory& src,
                                           const hidl_vec<uint32_t>& dimensions) {
    auto result = adapter::copyFrom(kBuffer, src, dimensions);
    if (!result.has_value()) {
        const auto [message, code] = std::move(result).error();
        LOG(ERROR) << "adapter::Buffer::copyFrom failed with " << code << ": " << message;
        return V1_3::utils::convert(code).value();
    }
    return V1_3::ErrorStatus::NONE;
}

}  // namespace android::hardware::neuralnetworks::adapter
