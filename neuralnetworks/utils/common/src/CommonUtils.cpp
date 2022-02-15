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

#include "CommonUtils.h"

#include <android-base/logging.h>
#include <nnapi/Result.h>
#include <nnapi/SharedMemory.h>
#include <nnapi/TypeUtils.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>

#include <algorithm>
#include <any>
#include <functional>
#include <optional>
#include <variant>
#include <vector>

namespace android::hardware::neuralnetworks::utils {

nn::Capabilities::OperandPerformanceTable makeQuantized8PerformanceConsistentWithP(
        const nn::Capabilities::PerformanceInfo& float32Performance,
        const nn::Capabilities::PerformanceInfo& quantized8Performance) {
    // In Android P, most data types are treated as having the same performance as
    // TENSOR_QUANT8_ASYMM. This collection must be in sorted order.
    std::vector<nn::Capabilities::OperandPerformance> operandPerformances = {
            {.type = nn::OperandType::FLOAT32, .info = float32Performance},
            {.type = nn::OperandType::INT32, .info = quantized8Performance},
            {.type = nn::OperandType::UINT32, .info = quantized8Performance},
            {.type = nn::OperandType::TENSOR_FLOAT32, .info = float32Performance},
            {.type = nn::OperandType::TENSOR_INT32, .info = quantized8Performance},
            {.type = nn::OperandType::TENSOR_QUANT8_ASYMM, .info = quantized8Performance},
            {.type = nn::OperandType::OEM, .info = quantized8Performance},
            {.type = nn::OperandType::TENSOR_OEM_BYTE, .info = quantized8Performance},
    };
    return nn::Capabilities::OperandPerformanceTable::create(std::move(operandPerformances))
            .value();
}

}  // namespace android::hardware::neuralnetworks::utils
