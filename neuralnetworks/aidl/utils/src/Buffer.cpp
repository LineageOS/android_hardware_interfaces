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

#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include "Conversions.h"
#include "Utils.h"
#include "nnapi/hal/aidl/Conversions.h"

#include <memory>
#include <utility>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes.

namespace aidl::android::hardware::neuralnetworks::utils {

nn::GeneralResult<std::shared_ptr<const Buffer>> Buffer::create(
        std::shared_ptr<aidl_hal::IBuffer> buffer, nn::Request::MemoryDomainToken token) {
    if (buffer == nullptr) {
        return NN_ERROR() << "aidl_hal::utils::Buffer::create must have non-null buffer";
    }
    if (token == static_cast<nn::Request::MemoryDomainToken>(0)) {
        return NN_ERROR() << "aidl_hal::utils::Buffer::create must have non-zero token";
    }

    return std::make_shared<const Buffer>(PrivateConstructorTag{}, std::move(buffer), token);
}

Buffer::Buffer(PrivateConstructorTag /*tag*/, std::shared_ptr<aidl_hal::IBuffer> buffer,
               nn::Request::MemoryDomainToken token)
    : kBuffer(std::move(buffer)), kToken(token) {
    CHECK(kBuffer != nullptr);
    CHECK(kToken != static_cast<nn::Request::MemoryDomainToken>(0));
}

nn::Request::MemoryDomainToken Buffer::getToken() const {
    return kToken;
}

nn::GeneralResult<void> Buffer::copyTo(const nn::SharedMemory& dst) const {
    const auto aidlDst = NN_TRY(convert(dst));

    const auto ret = kBuffer->copyTo(aidlDst);
    HANDLE_ASTATUS(ret) << "IBuffer::copyTo failed";

    return {};
}

nn::GeneralResult<void> Buffer::copyFrom(const nn::SharedMemory& src,
                                         const nn::Dimensions& dimensions) const {
    const auto aidlSrc = NN_TRY(convert(src));
    const auto aidlDimensions = NN_TRY(toSigned(dimensions));

    const auto ret = kBuffer->copyFrom(aidlSrc, aidlDimensions);
    HANDLE_ASTATUS(ret) << "IBuffer::copyFrom failed";

    return {};
}

}  // namespace aidl::android::hardware::neuralnetworks::utils
