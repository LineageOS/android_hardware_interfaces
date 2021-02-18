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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_RESILIENT_BUFFER_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_RESILIENT_BUFFER_H

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

class ResilientBuffer final : public nn::IBuffer {
    struct PrivateConstructorTag {};

  public:
    using Factory = std::function<nn::GeneralResult<nn::SharedBuffer>()>;

    static nn::GeneralResult<std::shared_ptr<const ResilientBuffer>> create(Factory makeBuffer);

    explicit ResilientBuffer(PrivateConstructorTag tag, Factory makeBuffer,
                             nn::SharedBuffer buffer);

    nn::SharedBuffer getBuffer() const;
    nn::GeneralResult<nn::SharedBuffer> recover(const nn::IBuffer* failingBuffer) const;

    nn::Request::MemoryDomainToken getToken() const override;

    nn::GeneralResult<void> copyTo(const nn::SharedMemory& dst) const override;

    nn::GeneralResult<void> copyFrom(const nn::SharedMemory& src,
                                     const nn::Dimensions& dimensions) const override;

  private:
    const Factory kMakeBuffer;
    mutable std::mutex mMutex;
    mutable nn::SharedBuffer mBuffer GUARDED_BY(mMutex);
};

}  // namespace android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_RESILIENT_BUFFER_H
