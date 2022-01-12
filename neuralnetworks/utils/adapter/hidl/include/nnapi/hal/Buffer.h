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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_BUFFER_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_BUFFER_H

#include <android/hardware/neuralnetworks/1.3/IBuffer.h>
#include <android/hardware/neuralnetworks/1.3/types.h>
#include <nnapi/IBuffer.h>
#include <nnapi/Types.h>
#include <memory>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on HIDL interface
// lifetimes across processes and for protecting asynchronous calls across HIDL.

namespace android::hardware::neuralnetworks::adapter {

// Class that adapts nn::IBuffer to V1_3::IBuffer.
class Buffer final : public V1_3::IBuffer {
  public:
    explicit Buffer(nn::SharedBuffer buffer);

    Return<V1_3::ErrorStatus> copyTo(const hidl_memory& dst) override;
    Return<V1_3::ErrorStatus> copyFrom(const hidl_memory& src,
                                       const hidl_vec<uint32_t>& dimensions) override;

  private:
    const nn::SharedBuffer kBuffer;
};

}  // namespace android::hardware::neuralnetworks::adapter

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_BUFFER_H
