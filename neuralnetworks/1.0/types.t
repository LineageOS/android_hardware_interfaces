%% template file for generating types.hal.
%% see frameworks/ml/nn/tools/api/README.md.
/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.hardware.neuralnetworks@1.0;

%insert Operand_1.0_Comment
enum OperandType : int32_t {
%insert Operand_1.0

    /**
     * DEPRECATED. Since HAL version 1.2, extensions are the preferred
     * alternative to OEM operation and data types.
     *
     * OEM specific scalar value.
     */
    OEM                 = 10000,

    /**
     * DEPRECATED. Since HAL version 1.2, extensions are the preferred
     * alternative to OEM operation and data types.
     *
     * A tensor of OEM specific values.
     */
    TENSOR_OEM_BYTE     = 10001,
};

%insert Operation_1.0_Comment
enum OperationType : int32_t {
%insert Operation_1.0

    /**
     * DEPRECATED. Since NNAPI 1.2, extensions are the preferred alternative to
     * OEM operation and data types.
     *
     * This operation is OEM specific. It should only be used for OEM
     * applications.
     */
    OEM_OPERATION = 10000,
};

/**
 * Fused activation function types.
 */
enum FusedActivationFunc : int32_t {
    NONE  = 0,
    RELU  = 1,
    RELU1 = 2,
    RELU6 = 3,
};

%insert OperandLifeTime

%insert DeviceStatus

%insert PerformanceInfo

%insert Capabilities

%insert DataLocation

%insert Operand

%insert Operation

%insert Model

%insert RequestArgument

%insert Request

/**
 * Return status of a function.
 */
enum ErrorStatus : int32_t {
    NONE,
    DEVICE_UNAVAILABLE,
    GENERAL_FAILURE,
    OUTPUT_INSUFFICIENT_SIZE,
    INVALID_ARGUMENT,
};
