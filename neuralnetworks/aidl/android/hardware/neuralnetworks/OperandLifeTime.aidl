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

package android.hardware.neuralnetworks;

/**
 * How an operand is used.
 */
@VintfStability
@Backing(type="int")
enum OperandLifeTime {
    /**
     * The operand is internal to the model. It's created by an operation and consumed by other
     * operations. It must be an output operand of exactly one operation.
     */
    TEMPORARY_VARIABLE,
    /**
     * The operand is an input of a subgraph. It must not be an output operand of any operation.
     *
     * An operand can't be both input and output of a subgraph.
     */
    SUBGRAPH_INPUT,
    /**
     * The operand is an output of a subgraph. It must be an output operand of exactly one
     * operation.
     *
     * An operand can't be both input and output of a subgraph.
     */
    SUBGRAPH_OUTPUT,
    /**
     * The operand is a constant found in Model.operandValues. It must not be an output operand of
     * any operation.
     */
    CONSTANT_COPY,
    /**
     * The operand is a constant that was specified via a Memory object. It must not be an output
     * operand of any operation.
     */
    CONSTANT_POOL,
    /**
     * The operand does not have a value. This is valid only for optional arguments of operations.
     */
    NO_VALUE,
    /**
     * The operand is a reference to a subgraph. It must be an input to one or more
     * {@link OperationType::IF} or {@link OperationType::WHILE} operations.
     */
    SUBGRAPH,
}
