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
package android.hardware.light;

@VintfStability
enum BrightnessMode {
    /**
     * Light brightness is managed by a user setting.
     */
    USER = 0,

    /**
     * Light brightness is managed by a light sensor. This is typically used
     * to control the display backlight, but not limited to it. HALs and
     * hardware implementations are free to support sensor for other lights or
     * none whatsoever.
     */
    SENSOR = 1,

    /**
     * Use a low-persistence mode for display backlights, where the pixel
     * color transition times are lowered.
     *
     * When set, the device driver must switch to a mode optimized for low display
     * persistence that is intended to be used when the device is being treated as a
     * head mounted display (HMD). The actual display brightness in this mode is
     * implementation dependent, and any value set for color in LightState may be
     * overridden by the HAL implementation.
     *
     * For an optimal HMD viewing experience, the display must meet the following
     * criteria in this mode:
     * - Gray-to-Gray, White-to-Black, and Black-to-White switching time must be ≤ 3 ms.
     * - The display must support low-persistence with ≤ 3.5 ms persistence.
     *   Persistence is defined as the amount of time for which a pixel is
     *   emitting light for a single frame.
     * - Any "smart panel" or other frame buffering options that increase display
     *   latency are disabled.
     * - Display brightness is set so that the display is still visible to the user
     *   under normal indoor lighting.
     * - The display must update at 60 Hz at least, but higher refresh rates are
     *   recommended for low latency.
     *
     */
    LOW_PERSISTENCE = 2,
}
