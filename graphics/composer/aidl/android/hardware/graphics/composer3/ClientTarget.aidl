/**
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.graphics.composer3;

import android.hardware.graphics.common.Dataspace;
import android.hardware.graphics.common.Rect;
import android.hardware.graphics.composer3.Buffer;

@VintfStability
parcelable ClientTarget {
    /**
     * Client target Buffer
     */
    Buffer buffer;

    /**
     * The dataspace of the buffer, as described in LayerCommand.dataspace.
     */
    Dataspace dataspace;

    /**
     * The surface damage regions.
     */
    Rect[] damage;

    /**
     * The HDR/SDR ratio.
     * Only meaningful for extended_range client targets to communicate the amount of HDR heaedroom
     * inside the client target. For floating point client targets, this means that for each color
     * channel the maximum SDR luminance is 1.0, and the maximum display relative luminance is
     * the hdrSdrRatio.
     * Note that this ratio is meant to be >= 1.0.
     */
    float hdrSdrRatio = 1.0f;
}
