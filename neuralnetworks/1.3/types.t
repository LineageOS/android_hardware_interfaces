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
import @1.2::OperandType;
import @1.2::OperationType;
import @1.2::SymmPerChannelQuantParams;

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

/**
 * Priority given to a prepared model for execution.
 */
enum Priority : int32_t {
    LOW,
    MEDIUM,
    HIGH,
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
     * its performance is treated as
     * { .execTime = FLT_MAX, .powerUsage = FLT_MAX }.
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
 * How an operand is used.
 */
enum OperandLifeTime : int32_t {
    /**
     * The operand is internal to the model. It's created by an operation and
     * consumed by other operations. It must be an output operand of
     * exactly one operation.
     */
    TEMPORARY_VARIABLE,

    /**
     * The operand is an input of a subgraph. It must not be an output
     * operand of any operation.
     *
     * An operand can't be both input and output of a subgraph.
     */
    SUBGRAPH_INPUT,

    /**
     * The operand is an output of a subgraph. It must be an output
     * operand of exactly one operation.
     *
     * An operand can't be both input and output of a subgraph.
     */
    SUBGRAPH_OUTPUT,

    /**
     * The operand is a constant found in Model.operandValues. It must
     * not be an output operand of any operation.
     */
    CONSTANT_COPY,

    /**
     * The operand is a constant that was specified via a Memory
     * object. It must not be an output operand of any operation.
     */
    CONSTANT_REFERENCE,

    /**
     * The operand does not have a value. This is valid only for optional
     * arguments of operations.
     */
    NO_VALUE,

    /**
     * The operand is a reference to a subgraph. It must be an input to one
     * or more {@link OperationType::IF} or {@link OperationType::WHILE}
     * operations.
     */
    SUBGRAPH,
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
     *     . The operand has lifetime SUBGRAPH_INPUT. Fully
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
     * If the lifetime is TEMPORARY_VARIABLE, SUBGRAPH_INPUT, SUBGRAPH_OUTPUT,
     * or NO_VALUE:
     * - All the fields must be 0.
     * If the lifetime is CONSTANT_COPY:
     * - location.poolIndex is 0.
     * - location.offset is the offset in bytes into Model.operandValues.
     * - location.length is set.
     * If the lifetime is CONSTANT_REFERENCE:
     * - location.poolIndex is set.
     * - location.offset is the offset in bytes into the specified pool.
     * - location.length is set.
     * If the lifetime is SUBGRAPH:
     * - location.poolIndex is 0.
     * - location.offset is the index of the referenced subgraph in
     *   {@link Model::referenced}.
     * - location.length is 0.
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
     * The top-level subgraph.
     */
    Subgraph main;

    /**
     * Referenced subgraphs.
     *
     * Each subgraph is referenced by the main subgraph or at least one other
     * referenced subgraph.
     *
     * There must be no reference cycles.
     */
    vec<Subgraph> referenced;

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
     * {@link @1.2::Model::ExtensionTypeEncoding::LOW_BITS_TYPE} bits of the
     * value correspond to the type ID within the extension and the high
     * {@link @1.2::Model::ExtensionTypeEncoding::HIGH_BITS_PREFIX} bits encode
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
    vec<@1.2::Model.ExtensionNameAndPrefix> extensionNameToPrefix;
};

/**
 * An excerpt of the execution graph.
 */
struct Subgraph {
    /**
     * All operands included in the subgraph.
     */
    vec<Operand> operands;

    /**
     * All operations included in the subgraph.
     *
     * The operations are sorted into execution order. Every operand
     * with lifetime SUBGRAPH_OUTPUT or TEMPORARY_VARIABLE must be
     * written before it is read.
     */
    vec<Operation> operations;

    /**
     * Input indexes of the subgraph. There must be at least one.
     *
     * Each value corresponds to the index of the operand in "operands".
     */
    vec<uint32_t> inputIndexes;

    /**
     * Output indexes of the subgraph. There must be at least one.
     *
     * Each value corresponds to the index of the operand in "operands".
     */
    vec<uint32_t> outputIndexes;
};

/**
 * A buffer descriptor. Describes the properties of a buffer.
 */
struct BufferDesc {
    /**
     * Dimensions of the buffer. May have unknown dimensions or rank. A buffer with some number
     * of unspecified dimensions is represented by setting each unspecified dimension to 0. A
     * buffer with unspecified rank is represented by providing an empty dimensions vector.
     */
    vec<uint32_t> dimensions;
};

/**
 * Describes a role of an input or output to a prepared model.
 */
struct BufferRole {
    /**
     * The index of the IPreparedModel within the "preparedModel" argument passed in
     * IDevice::allocate.
     */
    uint32_t modelIndex;

    /**
     * The index of the input or output operand.
     */
    uint32_t ioIndex;

    /**
     * A floating-point value within the range (0.0, 1.0]. Describes how likely the
     * buffer is to be used in the specified role. This is provided as a hint to
     * optimize the case when multiple roles prefer different buffer locations or data
     * layouts.
     */
    float frequency;
};

/**
 * Inputs to be sent to and outputs to be retrieved from a prepared model.
 *
 * A Request serves two primary tasks:
 * 1) Provides the input and output data to be used when executing the model.
 * 2) Specifies any updates to the input operand metadata that were left
 *    unspecified at model preparation time.
 *
 * An output must not overlap with any other output, with an input, or
 * with an operand of lifetime CONSTANT_REFERENCE.
 */
struct Request {
    /**
     * Input data and information to be used in the execution of a prepared
     * model.
     *
     * The index of the input corresponds to the index in Model.inputIndexes.
     *   E.g., input[i] corresponds to Model.inputIndexes[i].
     */
    vec<RequestArgument> inputs;

    /**
     * Output data and information to be used in the execution of a prepared
     * model.
     *
     * The index of the output corresponds to the index in Model.outputIndexes.
     *   E.g., output[i] corresponds to Model.outputIndexes[i].
     */
    vec<RequestArgument> outputs;

    /**
     * A memory pool.
     */
    safe_union MemoryPool {
        /**
         * Specifies a client-managed shared memory pool.
         */
        memory hidlMemory;

        /**
         * Specifies a driver-managed buffer. It is the token returned from IDevice::allocate,
         * and is specific to the IDevice object.
         */
        int32_t token;
    };

    /**
     * A collection of memory pools containing operand data for both the
     * inputs and the outputs to a model.
     */
    vec<MemoryPool> pools;
};

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
    uint64_t nanoseconds;
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
