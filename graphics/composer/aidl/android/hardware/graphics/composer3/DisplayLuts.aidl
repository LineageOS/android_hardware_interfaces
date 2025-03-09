/*
 * Copyright (C) 2024 The Android Open Source Project
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

import android.hardware.graphics.composer3.Luts;

/**
 * LUT (Look-Up Table) Interface for Color Transformation.
 *
 * This interface allows the HWC (Hardware Composer) to define and communicate Luts
 * to SurfaceFlinger.
 */
@VintfStability
parcelable DisplayLuts {
    /**
     * The display which the layerLuts list is for.
     */
    long display;

    parcelable LayerLut {
        /**
         * The layer that the HWC is requesting a LUT to be applied during GPU composition.
         */
        long layer;
        /**
         * Lut(s) specified by the HWC for given HDR layers that don't have Luts provided.
         */
        Luts luts;
    }

    LayerLut[] layerLuts;
}
