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

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/OperationTypes.h>
#include <nnapi/Types.h>
#include <type_traits>

namespace {

#define COMPARE_ENUMS_TYPES(lhsType, rhsType)                                                   \
    static_assert(                                                                              \
            std::is_same_v<                                                                     \
                    std::underlying_type_t<::android::hardware::neuralnetworks::V1_0::lhsType>, \
                    std::underlying_type_t<::android::nn::rhsType>>,                            \
            "::android::hardware::neuralnetworks::V1_0::" #lhsType                              \
            " does not have the same underlying type as ::android::nn::" #rhsType)

COMPARE_ENUMS_TYPES(OperandType, OperandType);
COMPARE_ENUMS_TYPES(OperationType, OperationType);
COMPARE_ENUMS_TYPES(ErrorStatus, ErrorStatus);
COMPARE_ENUMS_TYPES(OperandLifeTime, Operand::LifeTime);

#undef COMPARE_ENUMS_TYPES

#define COMPARE_ENUMS_FULL(lhsSymbol, rhsSymbol, lhsType, rhsType)                               \
    static_assert(                                                                               \
            static_cast<                                                                         \
                    std::underlying_type_t<::android::hardware::neuralnetworks::V1_0::lhsType>>( \
                    ::android::hardware::neuralnetworks::V1_0::lhsType::lhsSymbol) ==            \
                    static_cast<std::underlying_type_t<::android::nn::rhsType>>(                 \
                            ::android::nn::rhsType::rhsSymbol),                                  \
            "::android::hardware::neuralnetworks::V1_0::" #lhsType "::" #lhsSymbol               \
            " does not match ::android::nn::" #rhsType "::" #rhsSymbol)

#define COMPARE_ENUMS(symbol) COMPARE_ENUMS_FULL(symbol, symbol, OperandType, OperandType)

COMPARE_ENUMS(FLOAT32);
COMPARE_ENUMS(INT32);
COMPARE_ENUMS(UINT32);
COMPARE_ENUMS(TENSOR_FLOAT32);
COMPARE_ENUMS(TENSOR_INT32);
COMPARE_ENUMS(TENSOR_QUANT8_ASYMM);
COMPARE_ENUMS(OEM);
COMPARE_ENUMS(TENSOR_OEM_BYTE);

#undef COMPARE_ENUMS

#define COMPARE_ENUMS(symbol) COMPARE_ENUMS_FULL(symbol, symbol, OperationType, OperationType)

COMPARE_ENUMS(ADD);
COMPARE_ENUMS(AVERAGE_POOL_2D);
COMPARE_ENUMS(CONCATENATION);
COMPARE_ENUMS(CONV_2D);
COMPARE_ENUMS(DEPTHWISE_CONV_2D);
COMPARE_ENUMS(DEPTH_TO_SPACE);
COMPARE_ENUMS(DEQUANTIZE);
COMPARE_ENUMS(EMBEDDING_LOOKUP);
COMPARE_ENUMS(FLOOR);
COMPARE_ENUMS(FULLY_CONNECTED);
COMPARE_ENUMS(HASHTABLE_LOOKUP);
COMPARE_ENUMS(L2_NORMALIZATION);
COMPARE_ENUMS(L2_POOL_2D);
COMPARE_ENUMS(LOCAL_RESPONSE_NORMALIZATION);
COMPARE_ENUMS(LOGISTIC);
COMPARE_ENUMS(LSH_PROJECTION);
COMPARE_ENUMS(LSTM);
COMPARE_ENUMS(MAX_POOL_2D);
COMPARE_ENUMS(MUL);
COMPARE_ENUMS(RELU);
COMPARE_ENUMS(RELU1);
COMPARE_ENUMS(RELU6);
COMPARE_ENUMS(RESHAPE);
COMPARE_ENUMS(RESIZE_BILINEAR);
COMPARE_ENUMS(RNN);
COMPARE_ENUMS(SOFTMAX);
COMPARE_ENUMS(SPACE_TO_DEPTH);
COMPARE_ENUMS(SVDF);
COMPARE_ENUMS(TANH);
COMPARE_ENUMS(OEM_OPERATION);

#undef COMPARE_ENUMS

#define COMPARE_ENUMS(symbol) COMPARE_ENUMS_FULL(symbol, symbol, ErrorStatus, ErrorStatus)

COMPARE_ENUMS(NONE);
COMPARE_ENUMS(DEVICE_UNAVAILABLE);
COMPARE_ENUMS(GENERAL_FAILURE);
COMPARE_ENUMS(OUTPUT_INSUFFICIENT_SIZE);
COMPARE_ENUMS(INVALID_ARGUMENT);

#undef COMPARE_ENUMS

#define COMPARE_ENUMS(lhsSymbol, rhsSymbol) \
    COMPARE_ENUMS_FULL(lhsSymbol, rhsSymbol, OperandLifeTime, Operand::LifeTime)

COMPARE_ENUMS(TEMPORARY_VARIABLE, TEMPORARY_VARIABLE);
COMPARE_ENUMS(MODEL_INPUT, SUBGRAPH_INPUT);
COMPARE_ENUMS(MODEL_OUTPUT, SUBGRAPH_OUTPUT);
COMPARE_ENUMS(CONSTANT_COPY, CONSTANT_COPY);
COMPARE_ENUMS(CONSTANT_REFERENCE, CONSTANT_REFERENCE);
COMPARE_ENUMS(NO_VALUE, NO_VALUE);

#undef COMPARE_ENUMS

#undef COMPARE_ENUMS_FULL

}  // anonymous namespace
