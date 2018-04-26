/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef VTS_HAL_NEURALNETWORKS_V1_0_VTS_FUNCTIONAL_MODELS_H
#define VTS_HAL_NEURALNETWORKS_V1_0_VTS_FUNCTIONAL_MODELS_H

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include "TestHarness.h"

#include <android/hardware/neuralnetworks/1.0/types.h>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_0 {
namespace vts {
namespace functional {

using MixedTypedExample = test_helper::MixedTypedExampleType;

#define FOR_EACH_TEST_MODEL(FN)                          \
    FN(add_broadcast_quant8)                             \
    FN(add)                                              \
    FN(add_quant8)                                       \
    FN(avg_pool_float_1)                                 \
    FN(avg_pool_float_2)                                 \
    FN(avg_pool_float_3)                                 \
    FN(avg_pool_float_4)                                 \
    FN(avg_pool_float_5)                                 \
    FN(avg_pool_quant8_1)                                \
    FN(avg_pool_quant8_2)                                \
    FN(avg_pool_quant8_3)                                \
    FN(avg_pool_quant8_4)                                \
    FN(avg_pool_quant8_5)                                \
    FN(concat_float_1)                                   \
    FN(concat_float_2)                                   \
    FN(concat_float_3)                                   \
    FN(concat_quant8_1)                                  \
    FN(concat_quant8_2)                                  \
    FN(concat_quant8_3)                                  \
    FN(conv_1_h3_w2_SAME)                                \
    FN(conv_1_h3_w2_VALID)                               \
    FN(conv_3_h3_w2_SAME)                                \
    FN(conv_3_h3_w2_VALID)                               \
    FN(conv_float_2)                                     \
    FN(conv_float_channels)                              \
    FN(conv_float_channels_weights_as_inputs)            \
    FN(conv_float_large)                                 \
    FN(conv_float_large_weights_as_inputs)               \
    FN(conv_float)                                       \
    FN(conv_float_weights_as_inputs)                     \
    FN(conv_quant8_2)                                    \
    FN(conv_quant8_channels)                             \
    FN(conv_quant8_channels_weights_as_inputs)           \
    FN(conv_quant8_large)                                \
    FN(conv_quant8_large_weights_as_inputs)              \
    FN(conv_quant8)                                      \
    FN(conv_quant8_overflow)                             \
    FN(conv_quant8_overflow_weights_as_inputs)           \
    FN(conv_quant8_weights_as_inputs)                    \
    FN(depth_to_space_float_1)                           \
    FN(depth_to_space_float_2)                           \
    FN(depth_to_space_float_3)                           \
    FN(depth_to_space_quant8_1)                          \
    FN(depth_to_space_quant8_2)                          \
    FN(depthwise_conv2d_float_2)                         \
    FN(depthwise_conv2d_float_large_2)                   \
    FN(depthwise_conv2d_float_large_2_weights_as_inputs) \
    FN(depthwise_conv2d_float_large)                     \
    FN(depthwise_conv2d_float_large_weights_as_inputs)   \
    FN(depthwise_conv2d_float)                           \
    FN(depthwise_conv2d_float_weights_as_inputs)         \
    FN(depthwise_conv2d_quant8_2)                        \
    FN(depthwise_conv2d_quant8_large)                    \
    FN(depthwise_conv2d_quant8_large_weights_as_inputs)  \
    FN(depthwise_conv2d_quant8)                          \
    FN(depthwise_conv2d_quant8_weights_as_inputs)        \
    FN(depthwise_conv)                                   \
    FN(dequantize)                                       \
    FN(embedding_lookup)                                 \
    FN(floor)                                            \
    FN(fully_connected_float_2)                          \
    FN(fully_connected_float_large)                      \
    FN(fully_connected_float_large_weights_as_inputs)    \
    FN(fully_connected_float)                            \
    FN(fully_connected_float_weights_as_inputs)          \
    FN(fully_connected_quant8_2)                         \
    FN(fully_connected_quant8_large)                     \
    FN(fully_connected_quant8_large_weights_as_inputs)   \
    FN(fully_connected_quant8)                           \
    FN(fully_connected_quant8_weights_as_inputs)         \
    FN(hashtable_lookup_float)                           \
    FN(hashtable_lookup_quant8)                          \
    FN(l2_normalization_2)                               \
    FN(l2_normalization_large)                           \
    FN(l2_normalization)                                 \
    FN(l2_pool_float_2)                                  \
    FN(l2_pool_float_large)                              \
    FN(l2_pool_float)                                    \
    FN(local_response_norm_float_1)                      \
    FN(local_response_norm_float_2)                      \
    FN(local_response_norm_float_3)                      \
    FN(local_response_norm_float_4)                      \
    FN(logistic_float_1)                                 \
    FN(logistic_float_2)                                 \
    FN(logistic_quant8_1)                                \
    FN(logistic_quant8_2)                                \
    FN(lsh_projection_2)                                 \
    FN(lsh_projection)                                   \
    FN(lsh_projection_weights_as_inputs)                 \
    FN(lstm2)                                            \
    FN(lstm2_state2)                                     \
    FN(lstm2_state)                                      \
    FN(lstm3)                                            \
    FN(lstm3_state2)                                     \
    FN(lstm3_state3)                                     \
    FN(lstm3_state)                                      \
    FN(lstm)                                             \
    FN(lstm_state2)                                      \
    FN(lstm_state)                                       \
    FN(max_pool_float_1)                                 \
    FN(max_pool_float_2)                                 \
    FN(max_pool_float_3)                                 \
    FN(max_pool_float_4)                                 \
    FN(max_pool_quant8_1)                                \
    FN(max_pool_quant8_2)                                \
    FN(max_pool_quant8_3)                                \
    FN(max_pool_quant8_4)                                \
    FN(mobilenet_224_gender_basic_fixed)                 \
    FN(mobilenet_quantized)                              \
    FN(mul_broadcast_quant8)                             \
    FN(mul)                                              \
    FN(mul_quant8)                                       \
    FN(mul_relu)                                         \
    FN(relu1_float_1)                                    \
    FN(relu1_float_2)                                    \
    FN(relu1_quant8_1)                                   \
    FN(relu1_quant8_2)                                   \
    FN(relu6_float_1)                                    \
    FN(relu6_float_2)                                    \
    FN(relu6_quant8_1)                                   \
    FN(relu6_quant8_2)                                   \
    FN(relu_float_1)                                     \
    FN(relu_float_2)                                     \
    FN(relu_quant8_1)                                    \
    FN(relu_quant8_2)                                    \
    FN(reshape)                                          \
    FN(reshape_quant8)                                   \
    FN(reshape_quant8_weights_as_inputs)                 \
    FN(reshape_weights_as_inputs)                        \
    FN(resize_bilinear_2)                                \
    FN(resize_bilinear)                                  \
    FN(rnn)                                              \
    FN(rnn_state)                                        \
    FN(softmax_float_1)                                  \
    FN(softmax_float_2)                                  \
    FN(softmax_quant8_1)                                 \
    FN(softmax_quant8_2)                                 \
    FN(space_to_depth_float_1)                           \
    FN(space_to_depth_float_2)                           \
    FN(space_to_depth_float_3)                           \
    FN(space_to_depth_quant8_1)                          \
    FN(space_to_depth_quant8_2)                          \
    FN(svdf2)                                            \
    FN(svdf)                                             \
    FN(svdf_state)                                       \
    FN(tanh)

#define FORWARD_DECLARE_GENERATED_OBJECTS(function) \
    namespace function {                            \
    extern std::vector<MixedTypedExample> examples; \
    Model createTestModel();                        \
    }

FOR_EACH_TEST_MODEL(FORWARD_DECLARE_GENERATED_OBJECTS)

#undef FORWARD_DECLARE_GENERATED_OBJECTS

}  // namespace functional
}  // namespace vts
}  // namespace V1_0
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // VTS_HAL_NEURALNETWORKS_V1_0_VTS_FUNCTIONAL_MODELS_H
