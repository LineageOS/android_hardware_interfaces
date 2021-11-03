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

#ifndef ANDROID_FRAMEWORKS_ML_NN_COMMON_AIDL_HAL_UTILS_H
#define ANDROID_FRAMEWORKS_ML_NN_COMMON_AIDL_HAL_UTILS_H

#include <vector>

#include "AidlHalInterfaces.h"

namespace android {
namespace nn {

// Return a vector with one entry for each non-extension OperandType except
// SUBGRAPH, set to the specified PerformanceInfo value.  The vector will be
// sorted by OperandType.
//
// Control flow (OperandType::SUBGRAPH) operation performance is specified
// separately using Capabilities::ifPerformance and
// Capabilities::whilePerformance.
std::vector<aidl_hal::OperandPerformance> nonExtensionOperandPerformance(
        aidl_hal::PerformanceInfo perf);

// Update the vector entry corresponding to the specified OperandType with the
// specified PerformanceInfo value.  The vector must already have an entry for
// that OperandType, and must be sorted by OperandType.
void update(std::vector<aidl_hal::OperandPerformance>* operandPerformance,
            aidl_hal::OperandType type, aidl_hal::PerformanceInfo perf);

// Returns true if an operand type is an extension type.
bool isExtensionOperandType(aidl_hal::OperandType type);

// Returns true if an operand type is a scalar type.
bool isNonExtensionScalar(aidl_hal::OperandType type);

}  // namespace nn
}  // namespace android

#endif  // ANDROID_FRAMEWORKS_ML_NN_COMMON_AIDL_HAL_UTILS_H
