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

import android.hardware.neuralnetworks.OperationType;

/**
 * Describes one operation of the model's graph.
 */
@VintfStability
parcelable Operation {
    /**
     * The operation type.
     *
     * Besides the values listed in {@link OperationType}, any value above
     * {@link IDevice::OPERATION_TYPE_BASE_MAX} is possible and should be interpreted as an
     * extension type according to {@link Model::extensionNameToPrefix}.
     */
    OperationType type = OperationType.ADD;
    /**
     * Describes the table that contains the indexes of the inputs of the operation. The offset is
     * the index in the operandIndexes table.
     */
    int[] inputs;
    /**
     * Describes the table that contains the indexes of the outputs of the operation. The offset is
     * the index in the operandIndexes table.
     */
    int[] outputs;
}
