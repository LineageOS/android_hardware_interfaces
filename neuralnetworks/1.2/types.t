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

/**
 * Device types.
 *
 * The type of NNAPI device.
 */
enum DeviceType : int32_t {
    // Leaving 0 unused as it means unknown type in NDK NNAPI. There is no
    // HAL equivalent of unknown type and a 1.2 HAL implementation must belong
    // to one of the categories below.
    /** The device does not fall into any category below. */
    OTHER             = 1,
    /** The device runs NNAPI models on single or multi-core CPU. */
    CPU               = 2,
    /** The device can run NNAPI models and also accelerate graphics APIs such
      * as OpenGL ES and Vulkan. */
    GPU               = 3,
    /** Dedicated accelerator for Machine Learning workloads. */
    ACCELERATOR       = 4,
};

/**
 * The capabilities of a driver.
 *
 * Performance of an operation comes from the type of its first operand.
 * This represents performance for non extension operand types.
 */
struct Capabilities {
    /**
     * Driver performance when operating on float32 data but performing
     * calculations with range and/or precision as low as that of the IEEE
     * 754 16-bit floating-point format.
     */
    PerformanceInfo relaxedFloat32toFloat16PerformanceScalar;
    PerformanceInfo relaxedFloat32toFloat16PerformanceTensor;

    /**
     * Driver performance when operating on a particular data type.
     * In the case of float32 data, this is used when the calculations
     * are not relaxed.
     */
    struct OperandPerformance {
        OperandType type;
        PerformanceInfo info;
    };

    /**
     * Performance by operand type. Must be sorted by OperandType.
     * If a particular OperandType is not present in operandPerformance,
     * its performance is treated as { .execTime = FLT_MAX, .powerUsage = FLT_MAX }.
     */
    vec<OperandPerformance> operandPerformance;
};

/**
 * Describes one operation of the model's graph.
 */
struct Operation {
    /**
     * The operation type.
     *
     * Besides the values listed in {@link OperationType}, any value above
     * {@link OperationTypeRange::BASE_MAX} is possible and should be interpreted
     * as an extension type according to {@link Model::extensionNameToPrefix}.
     */
    OperationType type;

    /**
     * Describes the table that contains the indexes of the inputs of the
     * operation. The offset is the index in the operandIndexes table.
     */
    vec<uint32_t> inputs;

    /**
     * Describes the table that contains the indexes of the outputs of the
     * operation. The offset is the index in the operandIndexes table.
     */
    vec<uint32_t> outputs;
};

/**
 * Parameters for TENSOR_QUANT8_SYMM_PER_CHANNEL operand.
 */
struct SymmPerChannelQuantParams {
    /** Array of scaling values for each channel. Each value must be greater than zero. */
    vec<float> scales;
    /** Index of the channel dimension */
    uint32_t channelDim;
};

/**
 * Describes one operand of the model's graph.
 */
struct Operand {
    /**
     * The data type.
     *
     * Besides the values listed in {@link OperandType}, any value above
     * {@link OperandTypeRange::BASE_MAX} is possible and should be interpreted
     * as an extension type according to {@link Model::extensionNameToPrefix}.
     */
    OperandType type;

    /**
     * Dimensions of the operand.
     *
     * For a scalar operand, dimensions.size() must be 0.
     *
     * A tensor operand with all dimensions specified has "fully
     * specified" dimensions. Whenever possible (i.e., whenever the
     * dimensions are known at model construction time), a tensor
     * operand should have (but is not required to have) fully
     * specified dimensions, in order to enable the best possible
     * performance.
     *
     * If a tensor operand's dimensions are not fully specified, the
     * dimensions of the operand are deduced from the operand
     * dimensions and values of the operation for which that operand
     * is an output.
     *
     * In the following situations, a tensor operand's dimensions must
     * be fully specified:
     *
     *     . The operand has lifetime CONSTANT_COPY or
     *       CONSTANT_REFERENCE.
     *
     *     . The operand has lifetime MODEL_INPUT. Fully
     *       specified dimensions must either be present in the
     *       Operand or they must be provided in the corresponding
     *       RequestArgument.
     *       EXCEPTION: If the input is optional and omitted
     *       (by setting the hasNoValue field of the corresponding
     *       RequestArgument to true) then it need not have fully
     *       specified dimensions.
     *
     * A tensor operand with some number of unspecified dimensions is
     * represented by setting each unspecified dimension to 0.
     *
     * A tensor operand with unspecified rank is represented by providing
     * an empty dimensions vector.
     */
    vec<uint32_t> dimensions;

    /**
     * The number of times this operand appears as an operation input.
     *
     * (For example, if this operand appears once in one operation's
     * input list, and three times in another operation's input list,
     * then numberOfConsumers = 4.)
     */
    uint32_t numberOfConsumers;

    /**
     * Quantized scale of the operand.
     *
     * Only applicable if the operand is of type TENSOR_QUANT8_ASYMM or
     * TENSOR_INT32.
     */
    float scale;

    /**
     * Quantized zero-point offset of the operand.
     *
     * Only applicable if the operand is of type TENSOR_QUANT8_ASYMM.
     */
    int32_t zeroPoint;

    /**
     * How the operand is used.
     */
    OperandLifeTime lifetime;

    /**
     * Where to find the data for this operand.
     * If the lifetime is TEMPORARY_VARIABLE, MODEL_INPUT, MODEL_OUTPUT, or
     * NO_VALUE:
     * - All the fields must be 0.
     * If the lifetime is CONSTANT_COPY:
     * - location.poolIndex is 0.
     * - location.offset is the offset in bytes into Model.operandValues.
     * - location.length is set.
     * If the lifetime is CONSTANT_REFERENCE:
     * - location.poolIndex is set.
     * - location.offset is the offset in bytes into the specified pool.
     * - location.length is set.
     */
    DataLocation location;

    /**
     * Additional parameters specific to a particular operand type.
     */
    safe_union ExtraParams {
       /**
        * No additional parameters.
        */
       Monostate none;

       /**
        * Symmetric per-channel quantization parameters.
        *
        * Only applicable to operands of type TENSOR_QUANT8_SYMM_PER_CHANNEL.
        */
       SymmPerChannelQuantParams channelQuant;

       /**
        * Extension operand parameters.
        *
        * The framework treats this as an opaque data blob.
        * The format is up to individual extensions.
        */
       vec<uint8_t> extension;
    } extraParams;
};

/**
 * A Neural Network Model.
 *
 * This includes not only the execution graph, but also constant data such as
 * weights or scalars added at construction time. The only information that
 * may not be known is the shape of the input tensors.
 */
struct Model {
    /**
     * All operands included in the model.
     */
    vec<Operand> operands;

    /**
     * All operations included in the model.
     *
     * The operations are sorted into execution order. Every operand
     * with lifetime MODEL_OUTPUT or TEMPORARY_VARIABLE must be
     * written before it is read.
     */
    vec<Operation> operations;

    /**
     * Input indexes of the model. There must be at least one.
     *
     * Each value corresponds to the index of the operand in "operands".
     */
    vec<uint32_t> inputIndexes;

    /**
     * Output indexes of the model. There must be at least one.
     *
     * Each value corresponds to the index of the operand in "operands".
     */
    vec<uint32_t> outputIndexes;

    /**
     * A byte buffer containing operand data that were copied into the model.
     *
     * An operand's value must be located here if and only if Operand::lifetime
     * equals OperandLifeTime::CONSTANT_COPY.
     */
    vec<uint8_t> operandValues;

    /**
     * A collection of shared memory pools containing operand values.
     *
     * An operand's value must be located here if and only if Operand::lifetime
     * equals OperandLifeTime::CONSTANT_REFERENCE.
     */
    vec<memory> pools;

    /**
     * 'true' indicates TENSOR_FLOAT32 may be calculated with range and/or
     * precision as low as that of the IEEE 754 16-bit floating-point format.
     * 'false' indicates TENSOR_FLOAT32 must be calculated using at least the
     * range and precision of the IEEE 754 32-bit floating-point format.
     */
    bool relaxComputationFloat32toFloat16;

    /**
     * The mapping between extension names and prefixes of operand and
     * operation type values.
     *
     * An operand or operation whose numeric type value is above
     * {@link OperandTypeRange::BASE_MAX} or
     * {@link OperationTypeRange::BASE_MAX} respectively should be interpreted
     * as an extension operand. The low
     * {@link Model::ExtensionTypeEncoding::LOW_BITS_TYPE} bits of the value
     * correspond to the type ID within the extension and the high
     * {@link Model::ExtensionTypeEncoding::HIGH_BITS_PREFIX} bits encode
     * the "prefix", which maps uniquely to the extension name.
     *
     * For example, if a model contains an operation whose value is
     * 0xAAAABBBB and extensionNameToPrefix contains an entry with
     * prefix=0xAAAA and name="vendor.test.test_extension", then
     * the operation should be interpreted as the operation 0xBBBB
     * of the extension named vendor.test.test_extension.
     *
     * This is a one-to-one correspondence. That is, there must be at most one
     * prefix corresponding to each extension name and at most one extension
     * name corresponding to each prefix.
     */
    vec<ExtensionNameAndPrefix> extensionNameToPrefix;

    /**
     * A correspondence between an extension name and a prefix of operand and
     * operation type values.
     */
    struct ExtensionNameAndPrefix {
        /**
         * The extension name.
         *
         * See {@link Extension::name} for the format specification.
         */
        string name;

        /**
         * The unique extension identifier within the model.
         *
         * See {@link Model::extensionNameToPrefix}.
         */
        uint16_t prefix;
    };

    /**
     * Numeric values of extension operand and operation types have the
     * following structure:
     * - 16 high bits represent the "prefix", which corresponds uniquely to the
     *   extension name.
     * - 16 low bits represent the type ID within the extension.
     */
    enum ExtensionTypeEncoding : uint8_t {
        HIGH_BITS_PREFIX = 16,
        LOW_BITS_TYPE = 16,
    };
};

/**
 * Describes the shape information of an output operand after execution.
 */
struct OutputShape {
    /**
     * Dimensions of the operand.
     */
    vec<uint32_t> dimensions;

    /**
     * Whether the provided buffer size is sufficient for the output.
     */
    bool isSufficient;
};

/**
 * Specifies whether or not to measure timing information during execution.
 */
enum MeasureTiming : int32_t {
    NO  = 0,
    YES = 1,
};

/**

 * Timing information measured during execution. Each time is a duration from
 * the beginning of some task to the end of that task, including time when that
 * task is not active (for example, preempted by some other task, or
 * waiting for some resource to become available).
 *
 * Times are measured in microseconds.
 * When a time is not available, it must be reported as UINT64_MAX.
 */
struct Timing {
    /** Execution time on device (not driver, which runs on host processor). */
    uint64_t timeOnDevice;
    /** Execution time in driver (including time on device). */
    uint64_t timeInDriver;
};

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

/**
 * Information about an extension.
 */
struct Extension {
    /**
     * The extension name.
     *
     * The name must consist of lowercase latin letters, numbers, periods, and
     * underscore signs. The name must contain at least one period.
     *
     * The name must start with the reverse domain name of the vendor.
     *
     * Example: com.google.test_extension
     */
    string name;

    /**
     * Information about an extension operand type.
     */
    struct OperandTypeInformation {
        /**
         * The extension operand type.
         */
        uint16_t type;

        /**
         * Indicates whether the extension operand type represents a tensor or
         * a scalar.
         */
        bool isTensor;

        /**
         * The byte size of the operand (if scalar) or of a single element (if
         * tensor).
         */
        uint32_t byteSize;
    };

    /**
     * Information about operand types defined by the extension.
     */
    vec<OperandTypeInformation> operandTypes;
};
