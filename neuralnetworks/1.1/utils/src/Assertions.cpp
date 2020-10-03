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

#include <android/hardware/neuralnetworks/1.1/types.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/OperationTypes.h>
#include <nnapi/Types.h>
#include <type_traits>

namespace {

#define COMPARE_ENUMS_TYPES(type)                                                                  \
    static_assert(std::is_same_v<                                                                  \
                          std::underlying_type_t<::android::hardware::neuralnetworks::V1_1::type>, \
                          std::underlying_type_t<::android::nn::type>>,                            \
                  "::android::hardware::neuralnetworks::V1_1::" #type                              \
                  " does not have the same underlying type as ::android::nn::" #type)

COMPARE_ENUMS_TYPES(OperationType);
COMPARE_ENUMS_TYPES(ExecutionPreference);

#undef COMPARE_ENUMS_TYPES

#define COMPARE_ENUMS_FULL(symbol, type)                                                          \
    static_assert(                                                                                \
            static_cast<std::underlying_type_t<::android::hardware::neuralnetworks::V1_1::type>>( \
                    ::android::hardware::neuralnetworks::V1_1::type::symbol) ==                   \
                    static_cast<std::underlying_type_t<::android::nn::type>>(                     \
                            ::android::nn::type::symbol),                                         \
            "::android::hardware::neuralnetworks::V1_1::" #type "::" #symbol                      \
            " does not match ::android::nn::" #type "::" #symbol)

#define COMPARE_ENUMS(symbol) COMPARE_ENUMS_FULL(symbol, OperationType)

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
COMPARE_ENUMS(OEM_OPERATION);

#undef COMPARE_ENUMS

#define COMPARE_ENUMS(symbol) COMPARE_ENUMS_FULL(symbol, ExecutionPreference)

COMPARE_ENUMS(LOW_POWER);
COMPARE_ENUMS(FAST_SINGLE_ANSWER);
COMPARE_ENUMS(SUSTAINED_SPEED);

#undef COMPARE_ENUMS

#undef COMPARE_ENUMS_FULL

}  // anonymous namespace
