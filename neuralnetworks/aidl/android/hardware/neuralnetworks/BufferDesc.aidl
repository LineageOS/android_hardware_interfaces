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
 * A buffer descriptor. Describes the properties of a buffer.
 */
@VintfStability
parcelable BufferDesc {
    /**
     * Dimensions of the buffer. May have unknown dimensions or rank. A buffer with some number of
     * unspecified dimensions is represented by setting each unspecified dimension to 0. A buffer
     * with unspecified rank is represented by providing an empty dimensions vector.
     */
    int[] dimensions;
}
