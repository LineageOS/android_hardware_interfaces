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

#include <android-base/logging.h>
#include <android/hardware/neuralnetworks/1.2/types.h>

#include <functional>
#include <numeric>

namespace android {
namespace hardware {
namespace neuralnetworks {

uint32_t sizeOfData(V1_2::OperandType type) {
    switch (type) {
        case V1_2::OperandType::FLOAT32:
        case V1_2::OperandType::INT32:
        case V1_2::OperandType::UINT32:
        case V1_2::OperandType::TENSOR_FLOAT32:
        case V1_2::OperandType::TENSOR_INT32:
            return 4;
        case V1_2::OperandType::TENSOR_QUANT16_SYMM:
        case V1_2::OperandType::TENSOR_FLOAT16:
        case V1_2::OperandType::FLOAT16:
        case V1_2::OperandType::TENSOR_QUANT16_ASYMM:
            return 2;
        case V1_2::OperandType::TENSOR_QUANT8_ASYMM:
        case V1_2::OperandType::BOOL:
        case V1_2::OperandType::TENSOR_BOOL8:
        case V1_2::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
        case V1_2::OperandType::TENSOR_QUANT8_SYMM:
            return 1;
        default:
            CHECK(false) << "Invalid OperandType " << static_cast<uint32_t>(type);
            return 0;
    }
}

static bool isTensor(V1_2::OperandType type) {
    switch (type) {
        case V1_2::OperandType::FLOAT32:
        case V1_2::OperandType::INT32:
        case V1_2::OperandType::UINT32:
        case V1_2::OperandType::FLOAT16:
        case V1_2::OperandType::BOOL:
            return false;
        case V1_2::OperandType::TENSOR_FLOAT32:
        case V1_2::OperandType::TENSOR_INT32:
        case V1_2::OperandType::TENSOR_QUANT16_SYMM:
        case V1_2::OperandType::TENSOR_FLOAT16:
        case V1_2::OperandType::TENSOR_QUANT16_ASYMM:
        case V1_2::OperandType::TENSOR_QUANT8_ASYMM:
        case V1_2::OperandType::TENSOR_BOOL8:
        case V1_2::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
        case V1_2::OperandType::TENSOR_QUANT8_SYMM:
            return true;
        default:
            CHECK(false) << "Invalid OperandType " << static_cast<uint32_t>(type);
            return false;
    }
}

uint32_t sizeOfData(const V1_2::Operand& operand) {
    const uint32_t dataSize = sizeOfData(operand.type);
    if (isTensor(operand.type) && operand.dimensions.size() == 0) return 0;
    return std::accumulate(operand.dimensions.begin(), operand.dimensions.end(), dataSize,
                           std::multiplies<>{});
}

}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android
