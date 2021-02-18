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

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>
#include <android/hardware/neuralnetworks/1.2/types.h>
#include <android/hardware/neuralnetworks/1.3/IBuffer.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/1.0/Conversions.h>
#include <nnapi/hal/HandleError.h>

#include "Conversions.h"
#include "Utils.h"

#include <memory>
#include <utility>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes.

namespace android::hardware::neuralnetworks::V1_3::utils {

nn::GeneralResult<std::shared_ptr<const Buffer>> Buffer::create(
        sp<V1_3::IBuffer> buffer, nn::Request::MemoryDomainToken token) {
    if (buffer == nullptr) {
        return NN_ERROR() << "V1_3::utils::Buffer::create must have non-null buffer";
    }
    if (token == static_cast<nn::Request::MemoryDomainToken>(0)) {
        return NN_ERROR() << "V1_3::utils::Buffer::create must have non-zero token";
    }

    return std::make_shared<const Buffer>(PrivateConstructorTag{}, std::move(buffer), token);
}

Buffer::Buffer(PrivateConstructorTag /*tag*/, sp<V1_3::IBuffer> buffer,
               nn::Request::MemoryDomainToken token)
    : kBuffer(std::move(buffer)), kToken(token) {
    CHECK(kBuffer != nullptr);
    CHECK(kToken != static_cast<nn::Request::MemoryDomainToken>(0));
}

nn::Request::MemoryDomainToken Buffer::getToken() const {
    return kToken;
}

nn::GeneralResult<void> Buffer::copyTo(const nn::SharedMemory& dst) const {
    const auto hidlDst = NN_TRY(convert(dst));

    const auto ret = kBuffer->copyTo(hidlDst);
    const auto status = HANDLE_TRANSPORT_FAILURE(ret);
    HANDLE_HAL_STATUS(status) << "IBuffer::copyTo failed with " << toString(status);

    return {};
}

nn::GeneralResult<void> Buffer::copyFrom(const nn::SharedMemory& src,
                                         const nn::Dimensions& dimensions) const {
    const auto hidlSrc = NN_TRY(convert(src));
    const auto hidlDimensions = hidl_vec<uint32_t>(dimensions);

    const auto ret = kBuffer->copyFrom(hidlSrc, hidlDimensions);
    const auto status = HANDLE_TRANSPORT_FAILURE(ret);
    HANDLE_HAL_STATUS(status) << "IBuffer::copyFrom failed with " << toString(status);

    return {};
}

}  // namespace android::hardware::neuralnetworks::V1_3::utils
