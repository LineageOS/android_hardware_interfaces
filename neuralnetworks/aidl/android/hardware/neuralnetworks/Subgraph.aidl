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

import android.hardware.neuralnetworks.Operand;
import android.hardware.neuralnetworks.Operation;

/**
 * An excerpt of the execution graph.
 */
@VintfStability
parcelable Subgraph {
    /**
     * All operands included in the subgraph.
     */
    Operand[] operands;
    /**
     * All operations included in the subgraph.
     *
     * The operations are sorted into execution order. Every operand with lifetime SUBGRAPH_OUTPUT
     * or TEMPORARY_VARIABLE must be written before it is read.
     */
    Operation[] operations;
    /**
     * Input indexes of the subgraph. There must be at least one.
     *
     * Each value corresponds to the index of the operand in "operands".
     */
    int[] inputIndexes;
    /**
     * Output indexes of the subgraph. There must be at least one.
     *
     * Each value corresponds to the index of the operand in "operands".
     */
    int[] outputIndexes;
}
