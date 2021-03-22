%% template file for generating types.hal.
%% see frameworks/ml/nn/tools/api/README.md.
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

package android.hardware.neuralnetworks@1.2;

import @1.0::DataLocation;
import @1.0::ErrorStatus;
import @1.0::OperandLifeTime;
import @1.0::OperandType;
import @1.0::PerformanceInfo;
import @1.1::OperationType;

import android.hidl.safe_union@1.0::Monostate;

enum Constant : uint32_t {
    /**
     * The byte size of the cache token.
     */
    BYTE_SIZE_OF_CACHE_TOKEN = 32,

    /**
     * The maximum number of files for each type of cache in compilation caching.
     */
    MAX_NUMBER_OF_CACHE_FILES = 32,
};

enum OperandType : @1.0::OperandType {
%insert Operand_1.2
%insert OEMDeprecationAndOperandTypeRangeMaxComment
};

/**
 * The range of operand values in the OperandType enum.
 */
enum OperandTypeRange : uint32_t {
    BASE_MIN        = 0,
    FUNDAMENTAL_MIN = 0,
%insert Operand_1.2_MAX
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

    /**
     * DEPRECATED. Since NNAPI 1.2, extensions are the preferred alternative to
     * OEM operation and data types.
     *
     * This operation is OEM specific. It should only be used for OEM
     * applications.
     */
    OEM_OPERATION = @1.1::OperationType:OEM_OPERATION,
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
%insert Operation_1.2_MAX
    OEM_MIN         = 10000,
    OEM_MAX         = 10000,
    BASE_MAX        = 0xFFFF,
};

%insert DeviceType

%insert Capabilities

%insert Operation

%insert SymmPerChannelQuantParams

%insert Operand

%insert Model

%insert OutputShape

%insert MeasureTiming

%insert Timing

/**
 * FmqRequestDatum is a single element of a serialized representation of an
 * execution request (a {@link @1.0::Request} object and a {@link MeasureTiming}
 * value) which is sent across FastMessageQueue.
 *
 * The serialized representation for a particular execution is referred to later
 * in these descriptions as a 'packet'.
 *
 * FastMessageQueue can only pass HIDL-defined types that do not involve nested
 * buffers, handles, or interfaces.
 *
 * The request is serialized as follows:
 * 1) 'packetInformation'
 * 2) For each input operand:
 *    2.1) 'inputOperandInformation'
 *    2.2) For each dimension element of the operand:
 *         2.2.1) 'inputOperandDimensionValue'
 * 3) For each output operand:
 *    3.1) 'outputOperandInformation'
 *    3.2) For each dimension element of the operand:
 *         3.2.1) 'outputOperandDimensionValue'
 * 4) For each pool:
 *    4.1) 'poolIdentifier'
 * 5) 'measureTiming'
 */
safe_union FmqRequestDatum {
    /**
     * Type to describe the high-level layout of the packet.
     */
    struct PacketInformation {
        /**
         * How many elements the packet contains, including the
         * "packetInformation" datum.
         */
        uint32_t packetSize;

        /**
         * Number of input operands.
         */
        uint32_t numberOfInputOperands;

        /**
         * Number of output operands.
         */
        uint32_t numberOfOutputOperands;

        /**
         * Number of pool identifiers.
         */
        uint32_t numberOfPools;
    };

    /**
     * Type representing the information for each operand.
     */
    struct OperandInformation {
        /**
         * If true, the argument does not have a value. This can be used for
         * operations that take optional arguments. If true, the fields of
         * 'location' are set to 0, 'numberOfDimensions' is set to 0,  and the
         * dimensions information is omitted from the serialization.
         */
        bool hasNoValue;

        /**
         * The location within one of the memory pools passed in the Request.
         */
        DataLocation location;

        /**
         * Number of subsequent elements that belong to the dimensions vector.
         */
        uint32_t numberOfDimensions;
    };

    /**
     * packetInformation is the first element of the packet and describes the
     * remainder of the packet.
     */
    PacketInformation packetInformation;

    /**
     * Information for each input operand.
     */
    OperandInformation inputOperandInformation;

    /**
     * Element of the dimensions vector.
     */
    uint32_t inputOperandDimensionValue;

    /**
     * Information for each output operand.
     */
    OperandInformation outputOperandInformation;

    /**
     * Element of the dimensions vector.
     */
    uint32_t outputOperandDimensionValue;

    /**
     * Unique identifier for a pool.
     *
     * A {@link @1.0::Request} passes across one or more pools of shared memory
     * for the inputs and outputs of an execution. However, these memory pools
     * are not able to be sent across FastMessageQueue directly. Instead, the
     * producing side of the FMQ represents each different pool with a unique
     * identifier, and sends this identifier across the FMQ. Whenever the
     * consuming side of the FMQ needs the memory corresponding to this unique
     * identifier, it can pass the identifier to
     * {@link IBurstCallback::getMemories} to retreive the memory. Although this
     * HIDL Binder call is expensive compared to communication across FMQ, it is
     * only needed in the cases when the consumer does not recognize the unique
     * identifier.
     */
    int32_t poolIdentifier;

    /**
     * Specifies whether or not to measure duration of the execution. The
     * duration runs from the time the driver dequeues the request from a
     * FastMessageQueue to the time the driver enqueues results to a
     * FastMessageQueue.
     */
    MeasureTiming measureTiming;
};

/**
 * FmqResultDatum is a single element of a serialized representation of the
 * values returned from an execution ({@link @1.0::ErrorStatus},
 * vec<{@link OutputShape}>, and {@link Timing}) which is returned via
 * FastMessageQueue.
 *
 * The serialized representation for a particular execution is referred to later
 * in these descriptions as a 'packet'.
 *
 * FastMessageQueue can only pass HIDL-defined types that do not involve nested
 * buffers, handles, or interfaces.
 *
 * The execution return values ({@link @1.0::ErrorStatus} and
 * vec<{@link OutputShape}>) are serialized as follows:
 * 1) 'packetInformation'
 * 2) For each returned operand:
 *    2.1) 'operandInformation'
 *    2.2) For each dimension element of the operand:
 *         2.2.1) 'operandDimensionValue'
 * 3) 'executionTiming'
 */
safe_union FmqResultDatum {
    /**
     * Type to describe the high-level layout of the packet.
     */
    struct PacketInformation {
        /**
         * How many elements the packet contains, including the
         * "packetInformation" datum.
         */
        uint32_t packetSize;

        /**
         * Status of the execution.
         */
        ErrorStatus errorStatus;

        /**
         * Number of returned operands.
         */
        uint32_t numberOfOperands;
    };

    /**
     * Type representing the information for each operand.
     */
    struct OperandInformation {
        /**
         * Indicates whether the operand's output buffer is large enough to
         * store the operand's result data.
         */
        bool isSufficient;

        /**
         * Number of subsequent elements that belong to the dimensions vector.
         */
        uint32_t numberOfDimensions;
    };

    /**
     * packetInformation is the first element of the packet and describes the
     * remainder of the packet. It additionally includes the status of the
     * execution.
     */
    PacketInformation packetInformation;

    /**
     * Information for each returned operand.
     */
    OperandInformation operandInformation;

    /**
     * Element of the dimensions vector.
     */
    uint32_t operandDimensionValue;

    /**
     * Duration of execution. Unless measurement was requested and execution
     * succeeds, all times must be reported as UINT64_MAX. A driver may choose
     * to report any time as UINT64_MAX, indicating that measurement is not
     * available.
     */
    Timing executionTiming;
};

%insert Extension
