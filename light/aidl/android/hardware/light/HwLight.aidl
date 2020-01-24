/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.hardware.light;

import android.hardware.light.LightType;

/**
 * A description of a single light. Multiple lights can map to the same physical
 * LED. Separate physical LEDs are always represented by separate instances.
 */
@VintfStability
parcelable HwLight {
    /**
     * Integer ID used for controlling this light
     */
    int id;

    /**
     * For a group of lights of the same logical type, sorting by ordinal should
     * be give their physical order. No other meaning is carried by it.
     */
    int ordinal;

    /**
     * Logical type use of this light.
     */
    LightType type;
}
