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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_COMMON_UTILS_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_COMMON_UTILS_H

#include <nnapi/Result.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/Types.h>

#include <functional>
#include <vector>

// Shorthands
namespace android::hardware::neuralnetworks {
namespace hal = ::android::hardware::neuralnetworks;
}  // namespace android::hardware::neuralnetworks

// Shorthands
namespace aidl::android::hardware::neuralnetworks {
namespace aidl_hal = ::aidl::android::hardware::neuralnetworks;
namespace hal = ::android::hardware::neuralnetworks;
namespace nn = ::android::nn;
}  // namespace aidl::android::hardware::neuralnetworks

// Shorthands
namespace android::nn {
namespace hal = ::android::hardware::neuralnetworks;
namespace aidl_hal = ::aidl::android::hardware::neuralnetworks;
}  // namespace android::nn

namespace android::hardware::neuralnetworks::utils {

nn::Capabilities::OperandPerformanceTable makeQuantized8PerformanceConsistentWithP(
        const nn::Capabilities::PerformanceInfo& float32Performance,
        const nn::Capabilities::PerformanceInfo& quantized8Performance);

using nn::convertRequestFromPointerToShared;
using nn::flushDataFromPointerToShared;
using nn::hasNoPointerData;
using nn::RequestRelocation;

}  // namespace android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_COMMON_UTILS_H
