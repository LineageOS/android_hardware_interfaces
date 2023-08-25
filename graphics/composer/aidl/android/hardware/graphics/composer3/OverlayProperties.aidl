/**
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

package android.hardware.graphics.composer3;

@VintfStability
parcelable OverlayProperties {
    parcelable SupportedBufferCombinations {
        // List of pixelformats, standards, transfers and ranges dataspaces that can be used
        // together.
        // The pixelformats, standards, transfers and ranges stored inside are valid
        // combinations.
        // Dataspace identifies three components of colors - standard, transfer and
        // range.
        android.hardware.graphics.common.PixelFormat[] pixelFormats;
        android.hardware.graphics.common.Dataspace[] standards;
        android.hardware.graphics.common.Dataspace[] transfers;
        android.hardware.graphics.common.Dataspace[] ranges;
    }
    // Array of all valid pixelformat, standard, transfer and range combinations.
    // If all supported formats work with all standards, transfers and ranges,
    // then this list may only have 1 entry.
    // If some dataspaces, e.g. scRGB (STANDARD_BT709 | TRANSFER_SRGB | RANGE_EXTENDED),
    // only work with specific formats, then this list may contain more than 1 entry.
    // If some ranges, e.g. RANGE_LIMITED, only work with specific
    // formats/standards/transfers, then this list may contain more than 1 entry.
    SupportedBufferCombinations[] combinations;

    // True if the DPU is able to color manage at least two overlays
    // with different input colorspaces, false otherwise.
    boolean supportMixedColorSpaces;
}
