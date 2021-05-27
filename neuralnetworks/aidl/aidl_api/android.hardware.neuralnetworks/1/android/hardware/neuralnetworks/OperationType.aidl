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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.neuralnetworks;
@Backing(type="int") @VintfStability
enum OperationType {
  ADD = 0,
  AVERAGE_POOL_2D = 1,
  CONCATENATION = 2,
  CONV_2D = 3,
  DEPTHWISE_CONV_2D = 4,
  DEPTH_TO_SPACE = 5,
  DEQUANTIZE = 6,
  EMBEDDING_LOOKUP = 7,
  FLOOR = 8,
  FULLY_CONNECTED = 9,
  HASHTABLE_LOOKUP = 10,
  L2_NORMALIZATION = 11,
  L2_POOL_2D = 12,
  LOCAL_RESPONSE_NORMALIZATION = 13,
  LOGISTIC = 14,
  LSH_PROJECTION = 15,
  LSTM = 16,
  MAX_POOL_2D = 17,
  MUL = 18,
  RELU = 19,
  RELU1 = 20,
  RELU6 = 21,
  RESHAPE = 22,
  RESIZE_BILINEAR = 23,
  RNN = 24,
  SOFTMAX = 25,
  SPACE_TO_DEPTH = 26,
  SVDF = 27,
  TANH = 28,
  BATCH_TO_SPACE_ND = 29,
  DIV = 30,
  MEAN = 31,
  PAD = 32,
  SPACE_TO_BATCH_ND = 33,
  SQUEEZE = 34,
  STRIDED_SLICE = 35,
  SUB = 36,
  TRANSPOSE = 37,
  ABS = 38,
  ARGMAX = 39,
  ARGMIN = 40,
  AXIS_ALIGNED_BBOX_TRANSFORM = 41,
  BIDIRECTIONAL_SEQUENCE_LSTM = 42,
  BIDIRECTIONAL_SEQUENCE_RNN = 43,
  BOX_WITH_NMS_LIMIT = 44,
  CAST = 45,
  CHANNEL_SHUFFLE = 46,
  DETECTION_POSTPROCESSING = 47,
  EQUAL = 48,
  EXP = 49,
  EXPAND_DIMS = 50,
  GATHER = 51,
  GENERATE_PROPOSALS = 52,
  GREATER = 53,
  GREATER_EQUAL = 54,
  GROUPED_CONV_2D = 55,
  HEATMAP_MAX_KEYPOINT = 56,
  INSTANCE_NORMALIZATION = 57,
  LESS = 58,
  LESS_EQUAL = 59,
  LOG = 60,
  LOGICAL_AND = 61,
  LOGICAL_NOT = 62,
  LOGICAL_OR = 63,
  LOG_SOFTMAX = 64,
  MAXIMUM = 65,
  MINIMUM = 66,
  NEG = 67,
  NOT_EQUAL = 68,
  PAD_V2 = 69,
  POW = 70,
  PRELU = 71,
  QUANTIZE = 72,
  QUANTIZED_16BIT_LSTM = 73,
  RANDOM_MULTINOMIAL = 74,
  REDUCE_ALL = 75,
  REDUCE_ANY = 76,
  REDUCE_MAX = 77,
  REDUCE_MIN = 78,
  REDUCE_PROD = 79,
  REDUCE_SUM = 80,
  ROI_ALIGN = 81,
  ROI_POOLING = 82,
  RSQRT = 83,
  SELECT = 84,
  SIN = 85,
  SLICE = 86,
  SPLIT = 87,
  SQRT = 88,
  TILE = 89,
  TOPK_V2 = 90,
  TRANSPOSE_CONV_2D = 91,
  UNIDIRECTIONAL_SEQUENCE_LSTM = 92,
  UNIDIRECTIONAL_SEQUENCE_RNN = 93,
  RESIZE_NEAREST_NEIGHBOR = 94,
  QUANTIZED_LSTM = 95,
  IF = 96,
  WHILE = 97,
  ELU = 98,
  HARD_SWISH = 99,
  FILL = 100,
  RANK = 101,
}
