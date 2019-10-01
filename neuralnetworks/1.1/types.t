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

package android.hardware.neuralnetworks@1.1;

import @1.0::Operand;
import @1.0::OperationType;
import @1.0::PerformanceInfo;

/**
 * Operation types.
 *
 * The type of an operation in a model.
 */
enum OperationType : @1.0::OperationType {
%insert Operation_1.1
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

    /**
     * Driver performance when operating on float32 data but performing
     * calculations with range and/or precision as low as that of the IEEE
     * 754 16-bit floating-point format.
     */
    PerformanceInfo relaxedFloat32toFloat16Performance;
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
};

/**
 * Execution preferences.
 */
enum ExecutionPreference : int32_t {
    /**
     * Prefer executing in a way that minimizes battery drain.
     * This is desirable for compilations that will be executed often.
     */
    LOW_POWER = 0,
    /**
     * Prefer returning a single answer as fast as possible, even if this causes
     * more power consumption.
     */
    FAST_SINGLE_ANSWER = 1,
    /**
     * Prefer maximizing the throughput of successive frames, for example when
     * processing successive frames coming from the camera.
     */
    SUSTAINED_SPEED = 2,
};
