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

import android.hardware.graphics.composer3.DisplayAttribute;

/**
 * Display attributes queryable through getDisplayAttribute.
 */
@VintfStability
@Backing(type="int")
enum DisplayAttribute {
    INVALID = 0,
    /**
     * Dimensions in pixels
     */
    WIDTH = 1,
    HEIGHT = 2,
    /**
     * Vsync period in nanoseconds
     */
    VSYNC_PERIOD = 3,
    /**
     * Dots per thousand inches (DPI * 1000). Scaling by 1000 allows these
     * numbers to be stored in an int32_t without losing too much
     * precision. If the DPI for a configuration is unavailable or is
     * considered unreliable, the device may return UNSUPPORTED instead.
     */
    DPI_X = 4,
    DPI_Y = 5,

    // 6 is reserved for legacy interfaces

    /**
     * The configuration group ID (as int32_t) this config is associated to.
     * Switching between configurations within the same group may be done seamlessly
     * in some conditions via setActiveConfigWithConstraints. Configurations which
     * share the same config group are similar in all attributes except for the
     * vsync period.
     */
    CONFIG_GROUP = 7,
}
