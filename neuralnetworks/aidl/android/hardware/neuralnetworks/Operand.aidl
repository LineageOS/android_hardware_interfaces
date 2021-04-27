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

import android.hardware.neuralnetworks.DataLocation;
import android.hardware.neuralnetworks.OperandExtraParams;
import android.hardware.neuralnetworks.OperandLifeTime;
import android.hardware.neuralnetworks.OperandType;

/**
 * Describes one operand of the model's graph.
 */
@VintfStability
parcelable Operand {
    /**
     * The data type.
     *
     * Besides the values listed in {@link OperandType}, any value above
     * {@link IDevice::OPERAND_TYPE_BASE_MAX} is possible and should be interpreted as an extension
     * type according to {@link Model::extensionNameToPrefix}.
     */
    OperandType type = OperandType.FLOAT32;
    /**
     * Dimensions of the operand.
     *
     * For a scalar operand, dimensions.size() must be 0.
     *
     * A tensor operand with all dimensions specified has "fully specified" dimensions. Whenever
     * possible (i.e., whenever the dimensions are known at model construction time), a tensor
     * operand should have (but is not required to have) fully specified dimensions, in order to
     * enable the best possible performance.
     *
     * If a tensor operand's dimensions are not fully specified, the dimensions of the operand are
     * deduced from the operand dimensions and values of the operation for which that operand is an
     * output or from the corresponding {@link OperationType::IF} or {@link OperationType::WHILE}
     * operation input operand dimensions in the case of referenced subgraph input operands.
     *
     * In the following situations, a tensor operand's dimensions must be fully specified:
     *
     *     . The operand has lifetime CONSTANT_COPY or CONSTANT_POOL.
     *
     *     . The operand has lifetime SUBGRAPH_INPUT and belongs to the main subgraph. Fully
     *       specified dimensions must either be present in the Operand or they must be provided in
     *       the corresponding RequestArgument.
     *       EXCEPTION: If the input is optional and omitted (by setting the hasNoValue field of the
     *       corresponding RequestArgument to true) then it need not have fully specified
     *       dimensions.
     *
     * A tensor operand with some number of unspecified dimensions is represented by setting each
     * unspecified dimension to 0.
     *
     * A tensor operand with unspecified rank is represented by providing an empty dimensions
     * vector.
     */
    int[] dimensions;
    /**
     * Quantized scale of the operand.
     *
     * Must be 0 when not applicable to an operand type.
     *
     * See {@link OperandType}.
     */
    float scale;
    /**
     * Quantized zero-point offset of the operand.
     *
     * Must be 0 when not applicable to an operand type.
     *
     * See {@link OperandType}.
     */
    int zeroPoint;
    /**
     * How the operand is used.
     */
    OperandLifeTime lifetime = OperandLifeTime.TEMPORARY_VARIABLE;
    /**
     * Where to find the data for this operand.
     * If the lifetime is TEMPORARY_VARIABLE, SUBGRAPH_INPUT, SUBGRAPH_OUTPUT, or NO_VALUE:
     * - All the fields must be 0.
     * If the lifetime is CONSTANT_COPY:
     * - location.poolIndex is 0.
     * - location.offset is the offset in bytes into Model.operandValues.
     * - location.length is set.
     * If the lifetime is CONSTANT_POOL:
     * - location.poolIndex is set.
     * - location.offset is the offset in bytes into the specified pool.
     * - location.length is set.
     * If the lifetime is SUBGRAPH:
     * - location.poolIndex is 0.
     * - location.offset is the index of the referenced subgraph in {@link Model::referenced}.
     * - location.length is 0.
     */
    DataLocation location;
    /**
     * Additional parameters specific to a particular operand type.
     */
    @nullable OperandExtraParams extraParams;
}
