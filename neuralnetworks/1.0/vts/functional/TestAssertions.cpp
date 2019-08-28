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

#include <android/hardware/neuralnetworks/1.0/types.h>
#include "TestHarness.h"

namespace android::hardware::neuralnetworks::V1_0 {

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

CHECK_TEST_ENUM(OperandLifeTime, TEMPORARY_VARIABLE);
CHECK_TEST_ENUM(OperandLifeTime, MODEL_INPUT);
CHECK_TEST_ENUM(OperandLifeTime, MODEL_OUTPUT);
CHECK_TEST_ENUM(OperandLifeTime, CONSTANT_COPY);
CHECK_TEST_ENUM(OperandLifeTime, CONSTANT_REFERENCE);
CHECK_TEST_ENUM(OperandLifeTime, NO_VALUE);

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

#undef CHECK_TEST_ENUM

}  // namespace android::hardware::neuralnetworks::V1_0
