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

#ifndef VTS_HAL_NEURALNETWORKS_V1_1_VTS_FUNCTIONAL_MODELS_H
#define VTS_HAL_NEURALNETWORKS_V1_1_VTS_FUNCTIONAL_MODELS_H

#define LOG_TAG "neuralnetworks_hidl_hal_test"

#include "TestHarness.h"

#include <android/hardware/neuralnetworks/1.0/types.h>
#include <android/hardware/neuralnetworks/1.1/types.h>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace V1_1 {
namespace vts {
namespace functional {

using MixedTypedExample = test_helper::MixedTypedExampleType;

#define FOR_EACH_TEST_MODEL(FN)                                  \
    FN(add)                                                      \
    FN(add_broadcast_quant8)                                     \
    FN(add_quant8)                                               \
    FN(add_relaxed)                                              \
    FN(avg_pool_float_1)                                         \
    FN(avg_pool_float_1_relaxed)                                 \
    FN(avg_pool_float_2)                                         \
    FN(avg_pool_float_2_relaxed)                                 \
    FN(avg_pool_float_3)                                         \
    FN(avg_pool_float_3_relaxed)                                 \
    FN(avg_pool_float_4)                                         \
    FN(avg_pool_float_4_relaxed)                                 \
    FN(avg_pool_float_5)                                         \
    FN(avg_pool_float_5_relaxed)                                 \
    FN(avg_pool_quant8_1)                                        \
    FN(avg_pool_quant8_2)                                        \
    FN(avg_pool_quant8_3)                                        \
    FN(avg_pool_quant8_4)                                        \
    FN(avg_pool_quant8_5)                                        \
    FN(batch_to_space)                                           \
    FN(batch_to_space_float_1)                                   \
    FN(batch_to_space_float_1_relaxed)                           \
    FN(batch_to_space_quant8_1)                                  \
    FN(batch_to_space_relaxed)                                   \
    FN(concat_float_1)                                           \
    FN(concat_float_1_relaxed)                                   \
    FN(concat_float_2)                                           \
    FN(concat_float_2_relaxed)                                   \
    FN(concat_float_3)                                           \
    FN(concat_float_3_relaxed)                                   \
    FN(concat_quant8_1)                                          \
    FN(concat_quant8_2)                                          \
    FN(concat_quant8_3)                                          \
    FN(conv_1_h3_w2_SAME)                                        \
    FN(conv_1_h3_w2_SAME_relaxed)                                \
    FN(conv_1_h3_w2_VALID)                                       \
    FN(conv_1_h3_w2_VALID_relaxed)                               \
    FN(conv_3_h3_w2_SAME)                                        \
    FN(conv_3_h3_w2_SAME_relaxed)                                \
    FN(conv_3_h3_w2_VALID)                                       \
    FN(conv_3_h3_w2_VALID_relaxed)                               \
    FN(conv_float)                                               \
    FN(conv_float_2)                                             \
    FN(conv_float_2_relaxed)                                     \
    FN(conv_float_channels)                                      \
    FN(conv_float_channels_relaxed)                              \
    FN(conv_float_channels_weights_as_inputs)                    \
    FN(conv_float_channels_weights_as_inputs_relaxed)            \
    FN(conv_float_large)                                         \
    FN(conv_float_large_relaxed)                                 \
    FN(conv_float_large_weights_as_inputs)                       \
    FN(conv_float_large_weights_as_inputs_relaxed)               \
    FN(conv_float_relaxed)                                       \
    FN(conv_float_weights_as_inputs)                             \
    FN(conv_float_weights_as_inputs_relaxed)                     \
    FN(conv_quant8)                                              \
    FN(conv_quant8_2)                                            \
    FN(conv_quant8_channels)                                     \
    FN(conv_quant8_channels_weights_as_inputs)                   \
    FN(conv_quant8_large)                                        \
    FN(conv_quant8_large_weights_as_inputs)                      \
    FN(conv_quant8_overflow)                                     \
    FN(conv_quant8_overflow_weights_as_inputs)                   \
    FN(conv_quant8_weights_as_inputs)                            \
    FN(depth_to_space_float_1)                                   \
    FN(depth_to_space_float_1_relaxed)                           \
    FN(depth_to_space_float_2)                                   \
    FN(depth_to_space_float_2_relaxed)                           \
    FN(depth_to_space_float_3)                                   \
    FN(depth_to_space_float_3_relaxed)                           \
    FN(depth_to_space_quant8_1)                                  \
    FN(depth_to_space_quant8_2)                                  \
    FN(depthwise_conv)                                           \
    FN(depthwise_conv2d_float)                                   \
    FN(depthwise_conv2d_float_2)                                 \
    FN(depthwise_conv2d_float_2_relaxed)                         \
    FN(depthwise_conv2d_float_large)                             \
    FN(depthwise_conv2d_float_large_2)                           \
    FN(depthwise_conv2d_float_large_2_relaxed)                   \
    FN(depthwise_conv2d_float_large_2_weights_as_inputs)         \
    FN(depthwise_conv2d_float_large_2_weights_as_inputs_relaxed) \
    FN(depthwise_conv2d_float_large_relaxed)                     \
    FN(depthwise_conv2d_float_large_weights_as_inputs)           \
    FN(depthwise_conv2d_float_large_weights_as_inputs_relaxed)   \
    FN(depthwise_conv2d_float_relaxed)                           \
    FN(depthwise_conv2d_float_weights_as_inputs)                 \
    FN(depthwise_conv2d_float_weights_as_inputs_relaxed)         \
    FN(depthwise_conv2d_quant8)                                  \
    FN(depthwise_conv2d_quant8_2)                                \
    FN(depthwise_conv2d_quant8_large)                            \
    FN(depthwise_conv2d_quant8_large_weights_as_inputs)          \
    FN(depthwise_conv2d_quant8_weights_as_inputs)                \
    FN(depthwise_conv_relaxed)                                   \
    FN(dequantize)                                               \
    FN(dequantize_relaxed)                                       \
    FN(div)                                                      \
    FN(div_broadcast_float)                                      \
    FN(div_broadcast_float_relaxed)                              \
    FN(div_relaxed)                                              \
    FN(embedding_lookup)                                         \
    FN(embedding_lookup_relaxed)                                 \
    FN(floor)                                                    \
    FN(floor_relaxed)                                            \
    FN(fully_connected_float)                                    \
    FN(fully_connected_float_2)                                  \
    FN(fully_connected_float_2_relaxed)                          \
    FN(fully_connected_float_4d_simple)                          \
    FN(fully_connected_float_4d_simple_relaxed)                  \
    FN(fully_connected_float_large)                              \
    FN(fully_connected_float_large_relaxed)                      \
    FN(fully_connected_float_large_weights_as_inputs)            \
    FN(fully_connected_float_large_weights_as_inputs_relaxed)    \
    FN(fully_connected_float_relaxed)                            \
    FN(fully_connected_float_weights_as_inputs)                  \
    FN(fully_connected_float_weights_as_inputs_relaxed)          \
    FN(fully_connected_quant8)                                   \
    FN(fully_connected_quant8_2)                                 \
    FN(fully_connected_quant8_large)                             \
    FN(fully_connected_quant8_large_weights_as_inputs)           \
    FN(fully_connected_quant8_weights_as_inputs)                 \
    FN(hashtable_lookup_float)                                   \
    FN(hashtable_lookup_float_relaxed)                           \
    FN(hashtable_lookup_quant8)                                  \
    FN(l2_normalization)                                         \
    FN(l2_normalization_2)                                       \
    FN(l2_normalization_2_relaxed)                               \
    FN(l2_normalization_large)                                   \
    FN(l2_normalization_large_relaxed)                           \
    FN(l2_normalization_relaxed)                                 \
    FN(l2_pool_float)                                            \
    FN(l2_pool_float_2)                                          \
    FN(l2_pool_float_2_relaxed)                                  \
    FN(l2_pool_float_large)                                      \
    FN(l2_pool_float_large_relaxed)                              \
    FN(l2_pool_float_relaxed)                                    \
    FN(local_response_norm_float_1)                              \
    FN(local_response_norm_float_1_relaxed)                      \
    FN(local_response_norm_float_2)                              \
    FN(local_response_norm_float_2_relaxed)                      \
    FN(local_response_norm_float_3)                              \
    FN(local_response_norm_float_3_relaxed)                      \
    FN(local_response_norm_float_4)                              \
    FN(local_response_norm_float_4_relaxed)                      \
    FN(logistic_float_1)                                         \
    FN(logistic_float_1_relaxed)                                 \
    FN(logistic_float_2)                                         \
    FN(logistic_float_2_relaxed)                                 \
    FN(logistic_quant8_1)                                        \
    FN(logistic_quant8_2)                                        \
    FN(lsh_projection)                                           \
    FN(lsh_projection_2)                                         \
    FN(lsh_projection_2_relaxed)                                 \
    FN(lsh_projection_relaxed)                                   \
    FN(lsh_projection_weights_as_inputs)                         \
    FN(lsh_projection_weights_as_inputs_relaxed)                 \
    FN(lstm)                                                     \
    FN(lstm2)                                                    \
    FN(lstm2_relaxed)                                            \
    FN(lstm2_state)                                              \
    FN(lstm2_state2)                                             \
    FN(lstm2_state2_relaxed)                                     \
    FN(lstm2_state_relaxed)                                      \
    FN(lstm3)                                                    \
    FN(lstm3_relaxed)                                            \
    FN(lstm3_state)                                              \
    FN(lstm3_state2)                                             \
    FN(lstm3_state2_relaxed)                                     \
    FN(lstm3_state3)                                             \
    FN(lstm3_state3_relaxed)                                     \
    FN(lstm3_state_relaxed)                                      \
    FN(lstm_relaxed)                                             \
    FN(lstm_state)                                               \
    FN(lstm_state2)                                              \
    FN(lstm_state2_relaxed)                                      \
    FN(lstm_state_relaxed)                                       \
    FN(max_pool_float_1)                                         \
    FN(max_pool_float_1_relaxed)                                 \
    FN(max_pool_float_2)                                         \
    FN(max_pool_float_2_relaxed)                                 \
    FN(max_pool_float_3)                                         \
    FN(max_pool_float_3_relaxed)                                 \
    FN(max_pool_float_4)                                         \
    FN(max_pool_float_4_relaxed)                                 \
    FN(max_pool_quant8_1)                                        \
    FN(max_pool_quant8_2)                                        \
    FN(max_pool_quant8_3)                                        \
    FN(max_pool_quant8_4)                                        \
    FN(mean)                                                     \
    FN(mean_float_1)                                             \
    FN(mean_float_1_relaxed)                                     \
    FN(mean_float_2)                                             \
    FN(mean_float_2_relaxed)                                     \
    FN(mean_quant8_1)                                            \
    FN(mean_quant8_2)                                            \
    FN(mean_relaxed)                                             \
    FN(mobilenet_224_gender_basic_fixed)                         \
    FN(mobilenet_224_gender_basic_fixed_relaxed)                 \
    FN(mobilenet_quantized)                                      \
    FN(mul)                                                      \
    FN(mul_broadcast_quant8)                                     \
    FN(mul_quant8)                                               \
    FN(mul_relaxed)                                              \
    FN(mul_relu)                                                 \
    FN(mul_relu_relaxed)                                         \
    FN(pad)                                                      \
    FN(pad_float_1)                                              \
    FN(pad_float_1_relaxed)                                      \
    FN(pad_relaxed)                                              \
    FN(relu1_float_1)                                            \
    FN(relu1_float_1_relaxed)                                    \
    FN(relu1_float_2)                                            \
    FN(relu1_float_2_relaxed)                                    \
    FN(relu1_quant8_1)                                           \
    FN(relu1_quant8_2)                                           \
    FN(relu6_float_1)                                            \
    FN(relu6_float_1_relaxed)                                    \
    FN(relu6_float_2)                                            \
    FN(relu6_float_2_relaxed)                                    \
    FN(relu6_quant8_1)                                           \
    FN(relu6_quant8_2)                                           \
    FN(relu_float_1)                                             \
    FN(relu_float_1_relaxed)                                     \
    FN(relu_float_2)                                             \
    FN(relu_float_2_relaxed)                                     \
    FN(relu_quant8_1)                                            \
    FN(relu_quant8_2)                                            \
    FN(reshape)                                                  \
    FN(reshape_quant8)                                           \
    FN(reshape_quant8_weights_as_inputs)                         \
    FN(reshape_relaxed)                                          \
    FN(reshape_weights_as_inputs)                                \
    FN(reshape_weights_as_inputs_relaxed)                        \
    FN(resize_bilinear)                                          \
    FN(resize_bilinear_2)                                        \
    FN(resize_bilinear_2_relaxed)                                \
    FN(resize_bilinear_relaxed)                                  \
    FN(rnn)                                                      \
    FN(rnn_relaxed)                                              \
    FN(rnn_state)                                                \
    FN(rnn_state_relaxed)                                        \
    FN(softmax_float_1)                                          \
    FN(softmax_float_1_relaxed)                                  \
    FN(softmax_float_2)                                          \
    FN(softmax_float_2_relaxed)                                  \
    FN(softmax_quant8_1)                                         \
    FN(softmax_quant8_2)                                         \
    FN(space_to_batch)                                           \
    FN(space_to_batch_float_1)                                   \
    FN(space_to_batch_float_1_relaxed)                           \
    FN(space_to_batch_float_2)                                   \
    FN(space_to_batch_float_2_relaxed)                           \
    FN(space_to_batch_float_3)                                   \
    FN(space_to_batch_float_3_relaxed)                           \
    FN(space_to_batch_quant8_1)                                  \
    FN(space_to_batch_quant8_2)                                  \
    FN(space_to_batch_quant8_3)                                  \
    FN(space_to_batch_relaxed)                                   \
    FN(space_to_depth_float_1)                                   \
    FN(space_to_depth_float_1_relaxed)                           \
    FN(space_to_depth_float_2)                                   \
    FN(space_to_depth_float_2_relaxed)                           \
    FN(space_to_depth_float_3)                                   \
    FN(space_to_depth_float_3_relaxed)                           \
    FN(space_to_depth_quant8_1)                                  \
    FN(space_to_depth_quant8_2)                                  \
    FN(squeeze)                                                  \
    FN(squeeze_float_1)                                          \
    FN(squeeze_float_1_relaxed)                                  \
    FN(squeeze_quant8_1)                                         \
    FN(squeeze_relaxed)                                          \
    FN(strided_slice)                                            \
    FN(strided_slice_float_1)                                    \
    FN(strided_slice_float_10)                                   \
    FN(strided_slice_float_10_relaxed)                           \
    FN(strided_slice_float_11)                                   \
    FN(strided_slice_float_11_relaxed)                           \
    FN(strided_slice_float_1_relaxed)                            \
    FN(strided_slice_float_2)                                    \
    FN(strided_slice_float_2_relaxed)                            \
    FN(strided_slice_float_3)                                    \
    FN(strided_slice_float_3_relaxed)                            \
    FN(strided_slice_float_4)                                    \
    FN(strided_slice_float_4_relaxed)                            \
    FN(strided_slice_float_5)                                    \
    FN(strided_slice_float_5_relaxed)                            \
    FN(strided_slice_float_6)                                    \
    FN(strided_slice_float_6_relaxed)                            \
    FN(strided_slice_float_7)                                    \
    FN(strided_slice_float_7_relaxed)                            \
    FN(strided_slice_float_8)                                    \
    FN(strided_slice_float_8_relaxed)                            \
    FN(strided_slice_float_9)                                    \
    FN(strided_slice_float_9_relaxed)                            \
    FN(strided_slice_qaunt8_10)                                  \
    FN(strided_slice_qaunt8_11)                                  \
    FN(strided_slice_quant8_1)                                   \
    FN(strided_slice_quant8_2)                                   \
    FN(strided_slice_quant8_3)                                   \
    FN(strided_slice_quant8_4)                                   \
    FN(strided_slice_quant8_5)                                   \
    FN(strided_slice_quant8_6)                                   \
    FN(strided_slice_quant8_7)                                   \
    FN(strided_slice_quant8_8)                                   \
    FN(strided_slice_quant8_9)                                   \
    FN(strided_slice_relaxed)                                    \
    FN(sub)                                                      \
    FN(sub_broadcast_float)                                      \
    FN(sub_broadcast_float_relaxed)                              \
    FN(sub_relaxed)                                              \
    FN(svdf)                                                     \
    FN(svdf2)                                                    \
    FN(svdf2_relaxed)                                            \
    FN(svdf_relaxed)                                             \
    FN(svdf_state)                                               \
    FN(svdf_state_relaxed)                                       \
    FN(tanh)                                                     \
    FN(tanh_relaxed)                                             \
    FN(transpose)                                                \
    FN(transpose_float_1)                                        \
    FN(transpose_float_1_relaxed)                                \
    FN(transpose_quant8_1)                                       \
    FN(transpose_relaxed)

#define FORWARD_DECLARE_GENERATED_OBJECTS(function) \
    namespace function {                            \
    extern std::vector<MixedTypedExample> examples; \
    Model createTestModel();                        \
    }

FOR_EACH_TEST_MODEL(FORWARD_DECLARE_GENERATED_OBJECTS)

#undef FORWARD_DECLARE_GENERATED_OBJECTS

}  // namespace functional
}  // namespace vts
}  // namespace V1_1
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android

#endif  // VTS_HAL_NEURALNETWORKS_V1_1_VTS_FUNCTIONAL_MODELS_H
