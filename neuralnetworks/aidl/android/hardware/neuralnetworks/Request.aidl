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

import android.hardware.neuralnetworks.RequestArgument;
import android.hardware.neuralnetworks.RequestMemoryPool;

/**
 * Inputs to be sent to and outputs to be retrieved from a prepared model.
 *
 * A Request serves two primary tasks:
 * 1) Provides the input and output data to be used when executing the model.
 * 2) Specifies any updates to the input operand metadata that were left unspecified at model
 *    preparation time.
 *
 * An output must not overlap with any other output, with an input, or with an operand of lifetime
 * CONSTANT_POOL.
 */
@VintfStability
parcelable Request {
    /**
     * Input data and information to be used in the execution of a prepared model.
     *
     * The index of the input corresponds to the index in Model.main.inputIndexes.
     *   E.g., input[i] corresponds to Model.main.inputIndexes[i].
     */
    RequestArgument[] inputs;
    /**
     * Output data and information to be used in the execution of a prepared model.
     *
     * The index of the output corresponds to the index in Model.main.outputIndexes.
     *   E.g., output[i] corresponds to Model.main.outputIndexes[i].
     */
    RequestArgument[] outputs;
    /**
     * A collection of memory pools containing operand data for both the inputs and the outputs to a
     * model.
     */
    RequestMemoryPool[] pools;
}
