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

#include <android/hardware/neuralnetworks/1.3/types.h>
#include "ControlFlow.h"
#include "TestHarness.h"

namespace android::hardware::neuralnetworks::V1_3 {

static_assert(static_cast<uint64_t>(LoopTimeoutDurationNs::DEFAULT) ==
              nn::operation_while::kTimeoutNsDefault);
static_assert(static_cast<uint64_t>(LoopTimeoutDurationNs::MAXIMUM) ==
              nn::operation_while::kTimeoutNsMaximum);

// Make sure that the HIDL enums are compatible with the values defined in
// frameworks/ml/nn/tools/test_generator/test_harness/include/TestHarness.h.
using namespace test_helper;
#define CHECK_TEST_ENUM(EnumType, enumValue) \
    static_assert(static_cast<EnumType>(Test##EnumType::enumValue) == EnumType::enumValue)

CHECK_TEST_ENUM(OperandType, FLOAT32);
CHECK_TEST_ENUM(OperandType, INT32);
CHECK_TEST_ENUM(OperandType, UINT32);
CHECK_TEST_ENUM(OperandType, TENSOR_FLOAT32);
CHECK_TEST_ENUM(OperandType, TENSOR_INT32);
CHECK_TEST_ENUM(OperandType, TENSOR_QUANT8_ASYMM);
CHECK_TEST_ENUM(OperandType, BOOL);
CHECK_TEST_ENUM(OperandType, TENSOR_QUANT16_SYMM);
CHECK_TEST_ENUM(OperandType, TENSOR_FLOAT16);
CHECK_TEST_ENUM(OperandType, TENSOR_BOOL8);
CHECK_TEST_ENUM(OperandType, FLOAT16);
CHECK_TEST_ENUM(OperandType, TENSOR_QUANT8_SYMM_PER_CHANNEL);
CHECK_TEST_ENUM(OperandType, TENSOR_QUANT16_ASYMM);
CHECK_TEST_ENUM(OperandType, TENSOR_QUANT8_SYMM);
CHECK_TEST_ENUM(OperandType, TENSOR_QUANT8_ASYMM_SIGNED);

CHECK_TEST_ENUM(OperationType, ADD);
CHECK_TEST_ENUM(OperationType, AVERAGE_POOL_2D);
CHECK_TEST_ENUM(OperationType, CONCATENATION);
CHECK_TEST_ENUM(OperationType, CONV_2D);
CHECK_TEST_ENUM(OperationType, DEPTHWISE_CONV_2D);
CHECK_TEST_ENUM(OperationType, DEPTH_TO_SPACE);
CHECK_TEST_ENUM(OperationType, DEQUANTIZE);
CHECK_TEST_ENUM(OperationType, EMBEDDING_LOOKUP);
CHECK_TEST_ENUM(OperationType, FLOOR);
CHECK_TEST_ENUM(OperationType, FULLY_CONNECTED);
CHECK_TEST_ENUM(OperationType, HASHTABLE_LOOKUP);
CHECK_TEST_ENUM(OperationType, L2_NORMALIZATION);
CHECK_TEST_ENUM(OperationType, L2_POOL_2D);
CHECK_TEST_ENUM(OperationType, LOCAL_RESPONSE_NORMALIZATION);
CHECK_TEST_ENUM(OperationType, LOGISTIC);
CHECK_TEST_ENUM(OperationType, LSH_PROJECTION);
CHECK_TEST_ENUM(OperationType, LSTM);
CHECK_TEST_ENUM(OperationType, MAX_POOL_2D);
CHECK_TEST_ENUM(OperationType, MUL);
CHECK_TEST_ENUM(OperationType, RELU);
CHECK_TEST_ENUM(OperationType, RELU1);
CHECK_TEST_ENUM(OperationType, RELU6);
CHECK_TEST_ENUM(OperationType, RESHAPE);
CHECK_TEST_ENUM(OperationType, RESIZE_BILINEAR);
CHECK_TEST_ENUM(OperationType, RNN);
CHECK_TEST_ENUM(OperationType, SOFTMAX);
CHECK_TEST_ENUM(OperationType, SPACE_TO_DEPTH);
CHECK_TEST_ENUM(OperationType, SVDF);
CHECK_TEST_ENUM(OperationType, TANH);
CHECK_TEST_ENUM(OperationType, BATCH_TO_SPACE_ND);
CHECK_TEST_ENUM(OperationType, DIV);
CHECK_TEST_ENUM(OperationType, MEAN);
CHECK_TEST_ENUM(OperationType, PAD);
CHECK_TEST_ENUM(OperationType, SPACE_TO_BATCH_ND);
CHECK_TEST_ENUM(OperationType, SQUEEZE);
CHECK_TEST_ENUM(OperationType, STRIDED_SLICE);
CHECK_TEST_ENUM(OperationType, SUB);
CHECK_TEST_ENUM(OperationType, TRANSPOSE);
CHECK_TEST_ENUM(OperationType, ABS);
CHECK_TEST_ENUM(OperationType, ARGMAX);
CHECK_TEST_ENUM(OperationType, ARGMIN);
CHECK_TEST_ENUM(OperationType, AXIS_ALIGNED_BBOX_TRANSFORM);
CHECK_TEST_ENUM(OperationType, BIDIRECTIONAL_SEQUENCE_LSTM);
CHECK_TEST_ENUM(OperationType, BIDIRECTIONAL_SEQUENCE_RNN);
CHECK_TEST_ENUM(OperationType, BOX_WITH_NMS_LIMIT);
CHECK_TEST_ENUM(OperationType, CAST);
CHECK_TEST_ENUM(OperationType, CHANNEL_SHUFFLE);
CHECK_TEST_ENUM(OperationType, DETECTION_POSTPROCESSING);
CHECK_TEST_ENUM(OperationType, EQUAL);
CHECK_TEST_ENUM(OperationType, EXP);
CHECK_TEST_ENUM(OperationType, EXPAND_DIMS);
CHECK_TEST_ENUM(OperationType, GATHER);
CHECK_TEST_ENUM(OperationType, GENERATE_PROPOSALS);
CHECK_TEST_ENUM(OperationType, GREATER);
CHECK_TEST_ENUM(OperationType, GREATER_EQUAL);
CHECK_TEST_ENUM(OperationType, GROUPED_CONV_2D);
CHECK_TEST_ENUM(OperationType, HEATMAP_MAX_KEYPOINT);
CHECK_TEST_ENUM(OperationType, INSTANCE_NORMALIZATION);
CHECK_TEST_ENUM(OperationType, LESS);
CHECK_TEST_ENUM(OperationType, LESS_EQUAL);
CHECK_TEST_ENUM(OperationType, LOG);
CHECK_TEST_ENUM(OperationType, LOGICAL_AND);
CHECK_TEST_ENUM(OperationType, LOGICAL_NOT);
CHECK_TEST_ENUM(OperationType, LOGICAL_OR);
CHECK_TEST_ENUM(OperationType, LOG_SOFTMAX);
CHECK_TEST_ENUM(OperationType, MAXIMUM);
CHECK_TEST_ENUM(OperationType, MINIMUM);
CHECK_TEST_ENUM(OperationType, NEG);
CHECK_TEST_ENUM(OperationType, NOT_EQUAL);
CHECK_TEST_ENUM(OperationType, PAD_V2);
CHECK_TEST_ENUM(OperationType, POW);
CHECK_TEST_ENUM(OperationType, PRELU);
CHECK_TEST_ENUM(OperationType, QUANTIZE);
CHECK_TEST_ENUM(OperationType, QUANTIZED_16BIT_LSTM);
CHECK_TEST_ENUM(OperationType, RANDOM_MULTINOMIAL);
CHECK_TEST_ENUM(OperationType, REDUCE_ALL);
CHECK_TEST_ENUM(OperationType, REDUCE_ANY);
CHECK_TEST_ENUM(OperationType, REDUCE_MAX);
CHECK_TEST_ENUM(OperationType, REDUCE_MIN);
CHECK_TEST_ENUM(OperationType, REDUCE_PROD);
CHECK_TEST_ENUM(OperationType, REDUCE_SUM);
CHECK_TEST_ENUM(OperationType, ROI_ALIGN);
CHECK_TEST_ENUM(OperationType, ROI_POOLING);
CHECK_TEST_ENUM(OperationType, RSQRT);
CHECK_TEST_ENUM(OperationType, SELECT);
CHECK_TEST_ENUM(OperationType, SIN);
CHECK_TEST_ENUM(OperationType, SLICE);
CHECK_TEST_ENUM(OperationType, SPLIT);
CHECK_TEST_ENUM(OperationType, SQRT);
CHECK_TEST_ENUM(OperationType, TILE);
CHECK_TEST_ENUM(OperationType, TOPK_V2);
CHECK_TEST_ENUM(OperationType, TRANSPOSE_CONV_2D);
CHECK_TEST_ENUM(OperationType, UNIDIRECTIONAL_SEQUENCE_LSTM);
CHECK_TEST_ENUM(OperationType, UNIDIRECTIONAL_SEQUENCE_RNN);
CHECK_TEST_ENUM(OperationType, RESIZE_NEAREST_NEIGHBOR);

#undef CHECK_TEST_ENUM

}  // namespace android::hardware::neuralnetworks::V1_3
