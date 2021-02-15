/*
 * Copyright (C) 2021 The Android Open Source Project
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

import android.hardware.neuralnetworks.ErrorStatus;
import android.hardware.neuralnetworks.OutputShape;
import android.hardware.neuralnetworks.Timing;

/**
 * A result from running a synchronous execution of a prepared model.
 */
@VintfStability
parcelable ExecutionResult {
    /**
     * A value of "true" indicates that the execution was successful. A value of "false" indicates
     * the execution failed because at least one output operand buffer was not large enough to store
     * the corresponding output.
     */
    boolean outputSufficientSize;
    /**
     * A list of shape information of model output operands. The index in "outputShapes" corresponds
     * to the index of the output operand in the Request outputs vector.
     */
    OutputShape[] outputShapes;
    /**
     * Duration of execution. Unless measure is true and the execution is successful, all times must
     * be reported as -1. A driver may choose to report any time as -1, indicating that measurement
     * is not available.
     */
    Timing timing;
}
