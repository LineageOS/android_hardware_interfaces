%% template file for generating types.hal.
%% see frameworks/ml/nn/tools/api/README.md.
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

package android.hardware.neuralnetworks@1.3;

import @1.0::DataLocation;
import @1.0::ErrorStatus;
import @1.0::PerformanceInfo;
import @1.0::RequestArgument;
import @1.2::Model.ExtensionNameAndPrefix;
import @1.2::Model.ExtensionTypeEncoding;
import @1.2::Operand.ExtraParams;
import @1.2::OperandType;
import @1.2::OperationType;

import android.hidl.safe_union@1.0::Monostate;

enum OperandType : @1.2::OperandType {
%insert Operand_1.3
%insert OEMDeprecationAndOperandTypeRangeMaxComment
};

/**
 * The range of operand values in the OperandType enum.
 */
enum OperandTypeRange : uint32_t {
    BASE_MIN        = 0,
    FUNDAMENTAL_MIN = 0,
%insert Operand_1.3_MAX
    OEM_MIN         = 10000,
    OEM_MAX         = 10001,
    BASE_MAX        = 0xFFFF,
};

/**
 * Operation types.
 *
 * The type of an operation in a model.
 */
enum OperationType : int32_t {

%insert Operation_1.0

%insert Operation_1.1

%insert Operation_1.2

%insert Operation_1.3

    /**
     * DEPRECATED. Since NNAPI 1.2, extensions are the preferred alternative to
     * OEM operation and data types.
     *
     * This operation is OEM specific. It should only be used for OEM
     * applications.
     */
    OEM_OPERATION = @1.2::OperationType:OEM_OPERATION,
    /* ADDING A NEW FUNDAMENTAL OPERATION REQUIRES UPDATING THE VALUE OF
     * OperationTypeRange::FUNDAMENTAL_MAX.
     */
    /* ADDING A NEW OEM OPERATION REQUIRES UPDATING THE VALUE OF
     * OperationTypeRange::OEM_MAX.
     */
};

/**
 * The range of values in the OperationType enum.
 */
enum OperationTypeRange : uint32_t {
    BASE_MIN        = 0,
    FUNDAMENTAL_MIN = 0,
%insert Operation_1.3_MAX
    OEM_MIN         = 10000,
    OEM_MAX         = 10000,
    BASE_MAX        = 0xFFFF,
};

%insert Priority

%insert Capabilities

%insert Operation

%insert OperandLifeTime

%insert Operand

%insert Model

%insert Subgraph

%insert BufferDesc

%insert BufferRole

%insert Request

/**
 * Optional time point of the steady clock (as from std::chrono::steady_clock)
 * measured in nanoseconds.
 */
safe_union OptionalTimePoint {
    /** No time point provided. */
    Monostate none;

    /**
     * Time point of the steady clock (as from std::chrono::steady_clock)
     * measured in nanoseconds.
     */
    uint64_t nanosecondsSinceEpoch;
};

/**
 * Optional timeout duration measured in nanoseconds.
 */
safe_union OptionalTimeoutDuration {
    /** No time point provided. */
    Monostate none;

    /**
     * Timeout duration measured in nanoseconds.
     */
    uint64_t nanoseconds;
};

/**
 * Return status of a function.
 */
enum ErrorStatus : @1.0::ErrorStatus {
    /**
     * Failure because a deadline could not be met for a task, but future
     * deadlines may still be met for the same task after a short delay.
     */
    MISSED_DEADLINE_TRANSIENT,

    /**
     * Failure because a deadline could not be met for a task, and future
     * deadlines will likely also not be met for the same task even after a
     * short delay.
     */
    MISSED_DEADLINE_PERSISTENT,

    /**
     * Failure because of a resource limitation within the driver, but future
     * calls for the same task may still succeed after a short delay.
     */
    RESOURCE_EXHAUSTED_TRANSIENT,

    /**
     * Failure because of a resource limitation within the driver, and future
     * calls for the same task will likely also fail even after a short
     * delay.
     */
    RESOURCE_EXHAUSTED_PERSISTENT,
};

/**
 * Each {@link OperationType::WHILE} operation in the model has an implicit
 * execution timeout duration associated with it ("loop timeout duration").
 * This duration is configurable on a per-execution basis and must not exceed
 * 15 seconds. The default value is 2 seconds.
 */
enum LoopTimeoutDurationNs : uint64_t {
    DEFAULT = 2000000000,
    MAXIMUM = 15000000000,
};
