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

#include <aidl/android/hardware/neuralnetworks/DeviceType.h>
#include <aidl/android/hardware/neuralnetworks/ErrorStatus.h>
#include <aidl/android/hardware/neuralnetworks/ExecutionPreference.h>
#include <aidl/android/hardware/neuralnetworks/FusedActivationFunc.h>
#include <aidl/android/hardware/neuralnetworks/IDevice.h>
#include <aidl/android/hardware/neuralnetworks/OperandLifeTime.h>
#include <aidl/android/hardware/neuralnetworks/OperandType.h>
#include <aidl/android/hardware/neuralnetworks/OperationType.h>
#include <aidl/android/hardware/neuralnetworks/Priority.h>

#include <ControlFlow.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/OperationTypes.h>
#include <nnapi/Types.h>
#include <type_traits>

namespace {

#define COMPARE_ENUMS_TYPES(lhsType, rhsType)                                                   \
    static_assert(                                                                              \
            std::is_same_v<                                                                     \
                    std::underlying_type_t<::aidl::android::hardware::neuralnetworks::lhsType>, \
                    std::underlying_type_t<::android::nn::rhsType>>,                            \
            "::aidl::android::hardware::neuralnetworks::" #lhsType                              \
            " does not have the same underlying type as ::android::nn::" #rhsType)

COMPARE_ENUMS_TYPES(OperandType, OperandType);
COMPARE_ENUMS_TYPES(OperationType, OperationType);
COMPARE_ENUMS_TYPES(Priority, Priority);
COMPARE_ENUMS_TYPES(OperandLifeTime, Operand::LifeTime);
COMPARE_ENUMS_TYPES(ErrorStatus, ErrorStatus);

#undef COMPARE_ENUMS_TYPES

#define COMPARE_ENUMS_FULL(lhsSymbol, rhsSymbol, lhsType, rhsType)                               \
    static_assert(                                                                               \
            static_cast<                                                                         \
                    std::underlying_type_t<::aidl::android::hardware::neuralnetworks::lhsType>>( \
                    ::aidl::android::hardware::neuralnetworks::lhsType::lhsSymbol) ==            \
                    static_cast<std::underlying_type_t<::android::nn::rhsType>>(                 \
                            ::android::nn::rhsType::rhsSymbol),                                  \
            "::aidl::android::hardware::neuralnetworks::" #lhsType "::" #lhsSymbol               \
            " does not match ::android::nn::" #rhsType "::" #rhsSymbol)

#define COMPARE_ENUMS(symbol) COMPARE_ENUMS_FULL(symbol, symbol, OperandType, OperandType)

COMPARE_ENUMS(FLOAT32);
COMPARE_ENUMS(INT32);
COMPARE_ENUMS(UINT32);
COMPARE_ENUMS(TENSOR_FLOAT32);
COMPARE_ENUMS(TENSOR_INT32);
COMPARE_ENUMS(TENSOR_QUANT8_ASYMM);
COMPARE_ENUMS(BOOL);
COMPARE_ENUMS(TENSOR_QUANT16_SYMM);
COMPARE_ENUMS(TENSOR_FLOAT16);
COMPARE_ENUMS(TENSOR_BOOL8);
COMPARE_ENUMS(FLOAT16);
COMPARE_ENUMS(TENSOR_QUANT8_SYMM_PER_CHANNEL);
COMPARE_ENUMS(TENSOR_QUANT16_ASYMM);
COMPARE_ENUMS(TENSOR_QUANT8_SYMM);
COMPARE_ENUMS(TENSOR_QUANT8_ASYMM_SIGNED);
COMPARE_ENUMS(SUBGRAPH);

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
COMPARE_ENUMS(BATCH_TO_SPACE_ND);
COMPARE_ENUMS(DIV);
COMPARE_ENUMS(MEAN);
COMPARE_ENUMS(PAD);
COMPARE_ENUMS(SPACE_TO_BATCH_ND);
COMPARE_ENUMS(SQUEEZE);
COMPARE_ENUMS(STRIDED_SLICE);
COMPARE_ENUMS(SUB);
COMPARE_ENUMS(TRANSPOSE);
COMPARE_ENUMS(ABS);
COMPARE_ENUMS(ARGMAX);
COMPARE_ENUMS(ARGMIN);
COMPARE_ENUMS(AXIS_ALIGNED_BBOX_TRANSFORM);
COMPARE_ENUMS(BIDIRECTIONAL_SEQUENCE_LSTM);
COMPARE_ENUMS(BIDIRECTIONAL_SEQUENCE_RNN);
COMPARE_ENUMS(BOX_WITH_NMS_LIMIT);
COMPARE_ENUMS(CAST);
COMPARE_ENUMS(CHANNEL_SHUFFLE);
COMPARE_ENUMS(DETECTION_POSTPROCESSING);
COMPARE_ENUMS(EQUAL);
COMPARE_ENUMS(EXP);
COMPARE_ENUMS(EXPAND_DIMS);
COMPARE_ENUMS(GATHER);
COMPARE_ENUMS(GENERATE_PROPOSALS);
COMPARE_ENUMS(GREATER);
COMPARE_ENUMS(GREATER_EQUAL);
COMPARE_ENUMS(GROUPED_CONV_2D);
COMPARE_ENUMS(HEATMAP_MAX_KEYPOINT);
COMPARE_ENUMS(INSTANCE_NORMALIZATION);
COMPARE_ENUMS(LESS);
COMPARE_ENUMS(LESS_EQUAL);
COMPARE_ENUMS(LOG);
COMPARE_ENUMS(LOGICAL_AND);
COMPARE_ENUMS(LOGICAL_NOT);
COMPARE_ENUMS(LOGICAL_OR);
COMPARE_ENUMS(LOG_SOFTMAX);
COMPARE_ENUMS(MAXIMUM);
COMPARE_ENUMS(MINIMUM);
COMPARE_ENUMS(NEG);
COMPARE_ENUMS(NOT_EQUAL);
COMPARE_ENUMS(PAD_V2);
COMPARE_ENUMS(POW);
COMPARE_ENUMS(PRELU);
COMPARE_ENUMS(QUANTIZE);
COMPARE_ENUMS(QUANTIZED_16BIT_LSTM);
COMPARE_ENUMS(RANDOM_MULTINOMIAL);
COMPARE_ENUMS(REDUCE_ALL);
COMPARE_ENUMS(REDUCE_ANY);
COMPARE_ENUMS(REDUCE_MAX);
COMPARE_ENUMS(REDUCE_MIN);
COMPARE_ENUMS(REDUCE_PROD);
COMPARE_ENUMS(REDUCE_SUM);
COMPARE_ENUMS(ROI_ALIGN);
COMPARE_ENUMS(ROI_POOLING);
COMPARE_ENUMS(RSQRT);
COMPARE_ENUMS(SELECT);
COMPARE_ENUMS(SIN);
COMPARE_ENUMS(SLICE);
COMPARE_ENUMS(SPLIT);
COMPARE_ENUMS(SQRT);
COMPARE_ENUMS(TILE);
COMPARE_ENUMS(TOPK_V2);
COMPARE_ENUMS(TRANSPOSE_CONV_2D);
COMPARE_ENUMS(UNIDIRECTIONAL_SEQUENCE_LSTM);
COMPARE_ENUMS(UNIDIRECTIONAL_SEQUENCE_RNN);
COMPARE_ENUMS(RESIZE_NEAREST_NEIGHBOR);
COMPARE_ENUMS(QUANTIZED_LSTM);
COMPARE_ENUMS(IF);
COMPARE_ENUMS(WHILE);
COMPARE_ENUMS(ELU);
COMPARE_ENUMS(HARD_SWISH);
COMPARE_ENUMS(FILL);
COMPARE_ENUMS(RANK);

#undef COMPARE_ENUMS

#define COMPARE_ENUMS(symbol) COMPARE_ENUMS_FULL(symbol, symbol, Priority, Priority)

COMPARE_ENUMS(LOW);
COMPARE_ENUMS(MEDIUM);
COMPARE_ENUMS(HIGH);

#undef COMPARE_ENUMS

#define COMPARE_ENUMS(lhsSymbol, rhsSymbol) \
    COMPARE_ENUMS_FULL(lhsSymbol, rhsSymbol, OperandLifeTime, Operand::LifeTime)

COMPARE_ENUMS(TEMPORARY_VARIABLE, TEMPORARY_VARIABLE);
COMPARE_ENUMS(SUBGRAPH_INPUT, SUBGRAPH_INPUT);
COMPARE_ENUMS(SUBGRAPH_OUTPUT, SUBGRAPH_OUTPUT);
COMPARE_ENUMS(CONSTANT_COPY, CONSTANT_COPY);
COMPARE_ENUMS(CONSTANT_POOL, CONSTANT_REFERENCE);
COMPARE_ENUMS(NO_VALUE, NO_VALUE);
COMPARE_ENUMS(SUBGRAPH, SUBGRAPH);

#undef COMPARE_ENUMS

#define COMPARE_ENUMS(symbol) COMPARE_ENUMS_FULL(symbol, symbol, ErrorStatus, ErrorStatus)

COMPARE_ENUMS(NONE);
COMPARE_ENUMS(DEVICE_UNAVAILABLE);
COMPARE_ENUMS(GENERAL_FAILURE);
COMPARE_ENUMS(OUTPUT_INSUFFICIENT_SIZE);
COMPARE_ENUMS(INVALID_ARGUMENT);
COMPARE_ENUMS(MISSED_DEADLINE_TRANSIENT);
COMPARE_ENUMS(MISSED_DEADLINE_PERSISTENT);
COMPARE_ENUMS(RESOURCE_EXHAUSTED_TRANSIENT);
COMPARE_ENUMS(RESOURCE_EXHAUSTED_PERSISTENT);

#undef COMPARE_ENUMS

#define COMPARE_ENUMS(symbol) \
    COMPARE_ENUMS_FULL(symbol, symbol, ExecutionPreference, ExecutionPreference)

COMPARE_ENUMS(LOW_POWER);
COMPARE_ENUMS(FAST_SINGLE_ANSWER);
COMPARE_ENUMS(SUSTAINED_SPEED);

#undef COMPARE_ENUMS

#define COMPARE_ENUMS(symbol) COMPARE_ENUMS_FULL(symbol, symbol, DeviceType, DeviceType)

COMPARE_ENUMS(OTHER);
COMPARE_ENUMS(CPU);
COMPARE_ENUMS(GPU);
COMPARE_ENUMS(ACCELERATOR);

#undef COMPARE_ENUMS

#define COMPARE_ENUMS(symbol) \
    COMPARE_ENUMS_FULL(symbol, symbol, FusedActivationFunc, FusedActivationFunc)

COMPARE_ENUMS(NONE);
COMPARE_ENUMS(RELU);
COMPARE_ENUMS(RELU1);
COMPARE_ENUMS(RELU6);

#undef COMPARE_ENUMS

#undef COMPARE_ENUMS_FULL

#define COMPARE_CONSTANTS(halSymbol, canonicalSymbol)                     \
    static_assert(::aidl::android::hardware::neuralnetworks::halSymbol == \
                  ::android::nn::canonicalSymbol);

COMPARE_CONSTANTS(IDevice::BYTE_SIZE_OF_CACHE_TOKEN, kByteSizeOfCacheToken);
COMPARE_CONSTANTS(IDevice::MAX_NUMBER_OF_CACHE_FILES, kMaxNumberOfCacheFiles);
COMPARE_CONSTANTS(IDevice::EXTENSION_TYPE_HIGH_BITS_PREFIX, kExtensionPrefixBits - 1);
COMPARE_CONSTANTS(IDevice::EXTENSION_TYPE_LOW_BITS_TYPE, kExtensionTypeBits);
COMPARE_CONSTANTS(IPreparedModel::DEFAULT_LOOP_TIMEOUT_DURATION_NS,
                  operation_while::kTimeoutNsDefault);
COMPARE_CONSTANTS(IPreparedModel::MAXIMUM_LOOP_TIMEOUT_DURATION_NS,
                  operation_while::kTimeoutNsMaximum);

#undef COMPARE_CONSTANTS

}  // anonymous namespace
