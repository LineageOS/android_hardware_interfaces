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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_BUFFER_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_BUFFER_H

#include <aidl/android/hardware/neuralnetworks/BnBuffer.h>
#include <aidl/android/hardware/neuralnetworks/Memory.h>
#include <android/binder_auto_utils.h>
#include <nnapi/IBuffer.h>

#include <memory>
#include <vector>

// See hardware/interfaces/neuralnetworks/utils/README.md for more information on AIDL interface
// lifetimes across processes and for protecting asynchronous calls across AIDL.

namespace aidl::android::hardware::neuralnetworks::adapter {

// Class that adapts nn::IBuffer to BnBuffer.
class Buffer : public BnBuffer {
  public:
    explicit Buffer(::android::nn::SharedBuffer buffer);

    ndk::ScopedAStatus copyFrom(const Memory& src, const std::vector<int32_t>& dimensions) override;
    ndk::ScopedAStatus copyTo(const Memory& dst) override;

  private:
    const ::android::nn::SharedBuffer kBuffer;
};

}  // namespace aidl::android::hardware::neuralnetworks::adapter

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_ADAPTER_AIDL_BUFFER_H
