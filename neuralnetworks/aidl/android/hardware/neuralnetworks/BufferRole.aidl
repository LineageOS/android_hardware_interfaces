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
 * Describes a role of an input or output to a prepared model.
 */
@VintfStability
parcelable BufferRole {
    /**
     * The index of the IPreparedModel within the "preparedModel" argument passed in
     * IDevice::allocate.
     */
    int modelIndex;
    /**
     * The index of the input or output operand.
     */
    int ioIndex;
    /**
     * A floating-point value within the range (0.0, 1.0]. Describes how likely the buffer is to be
     * used in the specified role. This is provided as a hint to optimize the case when multiple
     * roles prefer different buffer locations or data layouts.
     */
    float probability;
}
