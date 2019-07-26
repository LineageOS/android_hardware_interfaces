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

import android.hardware.light.BrightnessMode;
import android.hardware.light.FlashMode;

/**
 * The parameters that can be set for a given light.
 *
 * Not all lights must support all parameters. If you
 * can do something backward-compatible, do it.
 */
@VintfStability
parcelable HwLightState {
    /**
     * The color of the LED in ARGB.
     *
     * The implementation of this in the HAL and hardware is a best-effort one.
     *   - If a light can only do red or green and blue is requested, green
     *     should be shown.
     *   - If only a brightness ramp is supported, then this formula applies:
     *      unsigned char brightness = ((77*((color>>16)&0x00ff))
     *              + (150*((color>>8)&0x00ff)) + (29*(color&0x00ff))) >> 8;
     *   - If only on and off are supported, 0 is off, anything else is on.
     *
     * The high byte should be ignored. Callers should set it to 0xff (which
     * would correspond to 255 alpha).
     */
    int color;

    /**
     * To flash the light at a given rate, set flashMode to FLASH_TIMED.
     */
    FlashMode flashMode;

    /**
     * flashOnMs should be set to the number of milliseconds to turn the
     * light on, before it's turned off.
     */
    int flashOnMs;

    /**
     * flashOfMs should be set to the number of milliseconds to turn the
     * light off, before it's turned back on.
     */
    int flashOffMs;

    BrightnessMode brightnessMode;
}
