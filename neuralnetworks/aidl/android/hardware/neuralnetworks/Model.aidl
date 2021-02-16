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

import android.hardware.neuralnetworks.ExtensionNameAndPrefix;
import android.hardware.neuralnetworks.Memory;
import android.hardware.neuralnetworks.Subgraph;

/**
 * A Neural Network Model.
 *
 * This includes not only the execution graph, but also constant data such as weights or scalars
 * added at construction time. The only information that may not be known is the shape of the input
 * tensors.
 */
@VintfStability
parcelable Model {
    /**
     * The top-level subgraph.
     */
    Subgraph main;
    /**
     * Referenced subgraphs.
     *
     * Each subgraph is referenced by the main subgraph or at least one other referenced subgraph.
     *
     * There must be no reference cycles.
     */
    Subgraph[] referenced;
    /**
     * A byte buffer containing operand data that were copied into the model.
     *
     * An operand's value must be located here if and only if Operand::lifetime equals
     * OperandLifeTime::CONSTANT_COPY.
     */
    byte[] operandValues;
    /**
     * A collection of shared memory pools containing operand values.
     *
     * An operand's value must be located here if and only if Operand::lifetime equals
     * OperandLifeTime::CONSTANT_POOL.
     */
    Memory[] pools;
    /**
     * 'true' indicates TENSOR_FLOAT32 may be calculated with range and/or precision as low as that
     * of the IEEE 754 16-bit floating-point format.
     * 'false' indicates TENSOR_FLOAT32 must be calculated using at least the range and precision of
     * the IEEE 754 32-bit floating-point format.
     */
    boolean relaxComputationFloat32toFloat16;
    /**
     * The mapping between extension names and prefixes of operand and operation type values.
     */
    ExtensionNameAndPrefix[] extensionNameToPrefix;
}
