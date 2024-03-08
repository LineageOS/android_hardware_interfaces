/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.media.c2;

import android.hardware.common.NativeHandle;
import android.hardware.media.c2.Params;

/**
 * Reference to a @ref BaseBlock within a @ref WorkBundle.
 *
 * `Block` contains additional attributes that `BaseBlock` does not. These
 * attributes may differ among `Block` objects that refer to the same
 * `BaseBlock` in the same `WorkBundle`.
 */
@VintfStability
parcelable Block {
    /**
     * Identity of a `BaseBlock` within a `WorkBundle`. This is an index into
     * #WorkBundle.baseBlocks.
     */
    int index;
    /**
     * Metadata associated with this `Block`.
     */
    Params meta;
    /**
     * Fence for synchronizing `Block` access.
     */
    NativeHandle fence;
}
