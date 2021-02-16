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

/**
 * Metadata information specifying the location of the input or output data and any updates to the
 * input or output operand.
 */
@VintfStability
parcelable RequestArgument {
    /**
     * If true, the argument does not have a value. This can be used for operations that take
     * optional arguments. If true, the fields of location are set to 0 and the dimensions vector is
     * left empty.
     */
    boolean hasNoValue;
    /**
     * The location within one of the memory pools passed in the Request.
     */
    DataLocation location;
    /**
     * Updated dimension information.
     *
     * If dimensions.size() > 0, dimension information was provided along with the argument. This
     * can be the case for models that accept inputs of varying size. This can't change the rank,
     * just the value of the dimensions that were unspecified in the model. If dimensions.size() >
     * 0, then all dimensions must be specified here; and any dimension that was specified in the
     * model must have the same value here.
     *
     * If the dimensions in the model are not fully specified, then they must be fully specified
     * here, unless hasNoValue is set to true. If the dimensions in the model are fully specified,
     * then either dimensions.size() may be 0, or the dimensions in the model must be identical to
     * the dimensions here.
     */
    int[] dimensions;
}
