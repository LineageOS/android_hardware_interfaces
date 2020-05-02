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

#include "1.3/Utils.h"

#include <iostream>
#include <numeric>
#include "android-base/logging.h"
#include "android/hardware/neuralnetworks/1.3/types.h"

namespace android::hardware::neuralnetworks {

uint32_t sizeOfData(V1_3::OperandType type) {
    switch (type) {
        case V1_3::OperandType::FLOAT32:
        case V1_3::OperandType::INT32:
        case V1_3::OperandType::UINT32:
        case V1_3::OperandType::TENSOR_FLOAT32:
        case V1_3::OperandType::TENSOR_INT32:
            return 4;
        case V1_3::OperandType::TENSOR_QUANT16_SYMM:
        case V1_3::OperandType::TENSOR_FLOAT16:
        case V1_3::OperandType::FLOAT16:
        case V1_3::OperandType::TENSOR_QUANT16_ASYMM:
            return 2;
        case V1_3::OperandType::TENSOR_QUANT8_ASYMM:
        case V1_3::OperandType::BOOL:
        case V1_3::OperandType::TENSOR_BOOL8:
        case V1_3::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
        case V1_3::OperandType::TENSOR_QUANT8_SYMM:
        case V1_3::OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
            return 1;
        case V1_3::OperandType::SUBGRAPH:
            return 0;
        default:
            CHECK(false) << "Invalid OperandType " << static_cast<uint32_t>(type);
            return 0;
    }
}

static bool isTensor(V1_3::OperandType type) {
    switch (type) {
        case V1_3::OperandType::FLOAT32:
        case V1_3::OperandType::INT32:
        case V1_3::OperandType::UINT32:
        case V1_3::OperandType::FLOAT16:
        case V1_3::OperandType::BOOL:
        case V1_3::OperandType::SUBGRAPH:
            return false;
        case V1_3::OperandType::TENSOR_FLOAT32:
        case V1_3::OperandType::TENSOR_INT32:
        case V1_3::OperandType::TENSOR_QUANT16_SYMM:
        case V1_3::OperandType::TENSOR_FLOAT16:
        case V1_3::OperandType::TENSOR_QUANT16_ASYMM:
        case V1_3::OperandType::TENSOR_QUANT8_ASYMM:
        case V1_3::OperandType::TENSOR_BOOL8:
        case V1_3::OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL:
        case V1_3::OperandType::TENSOR_QUANT8_SYMM:
        case V1_3::OperandType::TENSOR_QUANT8_ASYMM_SIGNED:
            return true;
        default:
            CHECK(false) << "Invalid OperandType " << static_cast<uint32_t>(type);
            return false;
    }
}

uint32_t sizeOfData(const V1_3::Operand& operand) {
    const uint32_t dataSize = sizeOfData(operand.type);
    if (isTensor(operand.type) && operand.dimensions.size() == 0) return 0;
    return std::accumulate(operand.dimensions.begin(), operand.dimensions.end(), dataSize,
                           std::multiplies<>{});
}

namespace V1_3 {

::std::ostream& operator<<(::std::ostream& os, ErrorStatus errorStatus) {
    return os << toString(errorStatus);
}

}  // namespace V1_3
}  // namespace android::hardware::neuralnetworks
