/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_NEURALNETWORKS_V1_2_UTILS_H
#define ANDROID_HARDWARE_NEURALNETWORKS_V1_2_UTILS_H

#include <android/hardware/neuralnetworks/1.2/types.h>

namespace android {
namespace hardware {
namespace neuralnetworks {

// Returns the amount of space needed to store a value of the specified type.
//
// Aborts if the specified type is an extension type or OEM type.
uint32_t sizeOfData(V1_2::OperandType type);

// Returns the amount of space needed to store a value of the dimensions and
// type of this operand. For a non-extension, non-OEM tensor with unspecified
// rank or at least one unspecified dimension, returns zero.
//
// Aborts if the specified type is an extension type or OEM type.
uint32_t sizeOfData(const V1_2::Operand& operand);

}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_NEURALNETWORKS_V1_2_UTILS_H
