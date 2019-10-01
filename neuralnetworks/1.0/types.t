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
     * The operand is an input of the model. It must not be an output
     * operand of any operation.
     *
     * An operand can't be both input and output of a model.
     */
    MODEL_INPUT,

    /**
     * The operand is an output of the model. It must be an output
     * operand of exactly one operation.
     *
     * An operand can't be both input and output of a model.
     */
    MODEL_OUTPUT,

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
};

/**
 * Status of a device.
 */
enum DeviceStatus : int32_t {
    AVAILABLE,
    BUSY,
    OFFLINE,
    UNKNOWN,
};

/**
 * Performance information for the reference workload.
 *
 * Used by a driver to report its performance characteristics.
 */
struct PerformanceInfo {
    /**
     * Ratio of the time taken by the driver to execute the
     * workload compared to the time the CPU would take for the
     * same workload. A lower number is better.
     */
    float execTime;

    /**
     * Ratio of the energy used by the driver compared to what
     * the CPU would use for doing the same workload. A lower number
     * is better.
     */
    float powerUsage;
};

/**
 * The capabilities of a driver.
 */
struct Capabilities {
    /**
     * Driver performance when operating on float32 data.
     */
    PerformanceInfo float32Performance;

    /**
     * Driver performance when operating on asymmetric 8-bit quantized data.
     */
    PerformanceInfo quantized8Performance;
};

/**
 * Describes the location of a data object.
 */
struct DataLocation {
    /**
     * The index of the memory pool where this location is found.
     */
    uint32_t poolIndex;

    /**
     * Offset in bytes from the start of the pool.
     */
    uint32_t offset;

    /**
     * The length of the data in bytes.
     */
    uint32_t length;
};

/**
 * Describes one operand of the model's graph.
 */
struct Operand {
    /**
     * Data type of the operand.
     */
    OperandType type;

    /**
     * Dimensions of the operand.
     *
     * For a scalar operand, dimensions.size() must be 0.
     *
     * For a tensor operand, dimensions.size() must be at least 1;
     * however, any of the dimensions may be unspecified.
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
     *     . The operand has lifetime MODEL_INPUT or MODEL_OUTPUT. Fully
     *       specified dimensions must either be present in the
     *       Operand or they must be provided in the corresponding
     *       RequestArgument.
     *       EXCEPTION: If the input or output is optional and omitted
     *       (by setting the hasNoValue field of the corresponding
     *       RequestArgument to true) then it need not have fully
     *       specified dimensions.
     *
     * A tensor operand with some number of unspecified dimensions is
     * represented by setting each unspecified dimension to 0.
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
};

/**
 * Describes one operation of the model's graph.
 */
struct Operation {
    /**
     * The operation type.
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
 * A Neural Network Model.
 *
 * This includes not only the execution graph, but also constant data such as
 * weights or scalars added at construction time. The only information that
 * might not be known is the shape of the input tensors.
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
};

/**
 * Metadata information specifying the location of the input or output data and
 * any updates to the input or output operand.
 */
struct RequestArgument {
    /**
     * If true, the argument does not have a value. This can be used for
     * operations that take optional arguments. If true, the fields of location
     * are set to 0 and the dimensions vector is left empty.
     */
    bool hasNoValue;

    /**
     * The location within one of the memory pools passed in the Request.
     */
    DataLocation location;

    /**
     * Updated dimension information.
     *
     * If dimensions.size() > 0, dimension information was provided
     * along with the argument. This can be the case for models that
     * accept inputs of varying size. This can't change the rank, just
     * the value of the dimensions that were unspecified in the
     * model. If dimensions.size() > 0, then all dimensions must be
     * specified here; and any dimension that was specified in the
     * model must have the same value here.
     *
     * If the dimensions in the model are not fully specified, then
     * they must be fully specified here, unless hasNoValue is set to
     * true. If the dimensions in the model are fully specified, then
     * either dimensions.size() may be 0, or the dimensions in the
     * model must be identical to the dimensions here.
     */
    vec<uint32_t> dimensions;
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
     * A collection of shared memory pools containing operand data for both the
     * inputs and the outputs to a model.
     */
    vec<memory> pools;
};

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
