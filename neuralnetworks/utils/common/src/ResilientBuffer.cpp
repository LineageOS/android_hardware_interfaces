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
#include <nnapi/Types.h>

#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::utils {

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
nn::SharedBuffer ResilientBuffer::recover(const nn::IBuffer* /*failingBuffer*/,
                                          bool /*blocking*/) const {
    std::lock_guard guard(mMutex);
    return mBuffer;
}

nn::Request::MemoryDomainToken ResilientBuffer::getToken() const {
    return getBuffer()->getToken();
}

nn::GeneralResult<void> ResilientBuffer::copyTo(const nn::Memory& dst) const {
    return getBuffer()->copyTo(dst);
}

nn::GeneralResult<void> ResilientBuffer::copyFrom(const nn::Memory& src,
                                                  const nn::Dimensions& dimensions) const {
    return getBuffer()->copyFrom(src, dimensions);
}

}  // namespace android::hardware::neuralnetworks::utils
