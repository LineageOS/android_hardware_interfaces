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

#include "ResilientBuffer.h"

#include <android-base/logging.h>
#include <android-base/thread_annotations.h>
#include <nnapi/IBuffer.h>
#include <nnapi/Result.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>

#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::utils {
namespace {

template <typename FnType>
auto protect(const ResilientBuffer& resilientBuffer, const FnType& fn)
        -> decltype(fn(*resilientBuffer.getBuffer())) {
    auto buffer = resilientBuffer.getBuffer();
    auto result = fn(*buffer);

    // Immediately return if device is not dead.
    if (result.has_value() || result.error().code != nn::ErrorStatus::DEAD_OBJECT) {
        return result;
    }

    // Attempt recovery and return if it fails.
    auto maybeBuffer = resilientBuffer.recover(buffer.get());
    if (!maybeBuffer.has_value()) {
        const auto& [resultErrorMessage, resultErrorCode] = result.error();
        const auto& [recoveryErrorMessage, recoveryErrorCode] = maybeBuffer.error();
        return nn::error(resultErrorCode)
               << resultErrorMessage << ", and failed to recover dead buffer with error "
               << recoveryErrorCode << ": " << recoveryErrorMessage;
    }
    buffer = std::move(maybeBuffer).value();

    return fn(*buffer);
}

}  // namespace

nn::GeneralResult<std::shared_ptr<const ResilientBuffer>> ResilientBuffer::create(
        Factory makeBuffer) {
    if (makeBuffer == nullptr) {
        return NN_ERROR(nn::ErrorStatus::INVALID_ARGUMENT)
               << "utils::ResilientBuffer::create must have non-empty makeBuffer";
    }
    auto buffer = NN_TRY(makeBuffer());
    CHECK(buffer != nullptr);
    return std::make_shared<const ResilientBuffer>(PrivateConstructorTag{}, std::move(makeBuffer),
                                                   std::move(buffer));
}

ResilientBuffer::ResilientBuffer(PrivateConstructorTag /*tag*/, Factory makeBuffer,
                                 nn::SharedBuffer buffer)
    : kMakeBuffer(std::move(makeBuffer)), mBuffer(std::move(buffer)) {
    CHECK(kMakeBuffer != nullptr);
    CHECK(mBuffer != nullptr);
}

nn::SharedBuffer ResilientBuffer::getBuffer() const {
    std::lock_guard guard(mMutex);
    return mBuffer;
}
nn::GeneralResult<nn::SharedBuffer> ResilientBuffer::recover(
        const nn::IBuffer* failingBuffer) const {
    std::lock_guard guard(mMutex);

    // Another caller updated the failing prepared model.
    if (mBuffer.get() != failingBuffer) {
        return mBuffer;
    }

    mBuffer = NN_TRY(kMakeBuffer());
    return mBuffer;
}

nn::Request::MemoryDomainToken ResilientBuffer::getToken() const {
    return getBuffer()->getToken();
}

nn::GeneralResult<void> ResilientBuffer::copyTo(const nn::SharedMemory& dst) const {
    const auto fn = [&dst](const nn::IBuffer& buffer) { return buffer.copyTo(dst); };
    return protect(*this, fn);
}

nn::GeneralResult<void> ResilientBuffer::copyFrom(const nn::SharedMemory& src,
                                                  const nn::Dimensions& dimensions) const {
    const auto fn = [&src, &dimensions](const nn::IBuffer& buffer) {
        return buffer.copyFrom(src, dimensions);
    };
    return protect(*this, fn);
}

}  // namespace android::hardware::neuralnetworks::utils
