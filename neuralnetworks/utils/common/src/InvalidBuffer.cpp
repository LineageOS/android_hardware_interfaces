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

#include "InvalidBuffer.h"

#include <nnapi/IBuffer.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <memory>
#include <utility>
#include <vector>

namespace android::hardware::neuralnetworks::utils {

nn::Request::MemoryDomainToken InvalidBuffer::getToken() const {
    return nn::Request::MemoryDomainToken{};
}

nn::GeneralResult<void> InvalidBuffer::copyTo(const nn::SharedMemory& /*dst*/) const {
    return NN_ERROR() << "InvalidBuffer";
}

nn::GeneralResult<void> InvalidBuffer::copyFrom(const nn::SharedMemory& /*src*/,
                                                const nn::Dimensions& /*dimensions*/) const {
    return NN_ERROR() << "InvalidBuffer";
}

}  // namespace android::hardware::neuralnetworks::utils
