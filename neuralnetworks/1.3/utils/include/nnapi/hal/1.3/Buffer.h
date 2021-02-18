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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_UTILS_BUFFER_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_UTILS_BUFFER_H

#include <android/hardware/neuralnetworks/1.3/IBuffer.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/IBuffer.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <memory>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes.

namespace android::hardware::neuralnetworks::V1_3::utils {

// Class that adapts V1_3::IBuffer to nn::IBuffer.
class Buffer final : public nn::IBuffer {
    struct PrivateConstructorTag {};

  public:
    static nn::GeneralResult<std::shared_ptr<const Buffer>> create(
            sp<V1_3::IBuffer> buffer, nn::Request::MemoryDomainToken token);

    Buffer(PrivateConstructorTag tag, sp<V1_3::IBuffer> buffer,
           nn::Request::MemoryDomainToken token);

    nn::Request::MemoryDomainToken getToken() const override;

    nn::GeneralResult<void> copyTo(const nn::SharedMemory& dst) const override;
    nn::GeneralResult<void> copyFrom(const nn::SharedMemory& src,
                                     const nn::Dimensions& dimensions) const override;

  private:
    const sp<V1_3::IBuffer> kBuffer;
    const nn::Request::MemoryDomainToken kToken;
};

}  // namespace android::hardware::neuralnetworks::V1_3::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_1_3_UTILS_BUFFER_H
