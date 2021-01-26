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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_BUFFER_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_BUFFER_H

#include <aidl/android/hardware/neuralnetworks/IBuffer.h>
#include <nnapi/IBuffer.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/hal/CommonUtils.h>
#include <memory>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes.

namespace aidl::android::hardware::neuralnetworks::utils {

// Class that adapts aidl_hal::IBuffer to  nn::IBuffer.
class Buffer final : public nn::IBuffer {
    struct PrivateConstructorTag {};

  public:
    static nn::GeneralResult<std::shared_ptr<const Buffer>> create(
            std::shared_ptr<aidl_hal::IBuffer> buffer, nn::Request::MemoryDomainToken token);

    Buffer(PrivateConstructorTag tag, std::shared_ptr<aidl_hal::IBuffer> buffer,
           nn::Request::MemoryDomainToken token);

    nn::Request::MemoryDomainToken getToken() const override;

    nn::GeneralResult<void> copyTo(const nn::SharedMemory& dst) const override;
    nn::GeneralResult<void> copyFrom(const nn::SharedMemory& src,
                                     const nn::Dimensions& dimensions) const override;

  private:
    const std::shared_ptr<aidl_hal::IBuffer> kBuffer;
    const nn::Request::MemoryDomainToken kToken;
};

}  // namespace aidl::android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_AIDL_UTILS_BUFFER_H
