/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.tv.mediaquality;

import android.hardware.graphics.common.PixelFormat;
import android.hardware.tv.mediaquality.AmbientBacklightSource;

@VintfStability
parcelable AmbientBacklightSettings {
    /**
     * The package name of the ambient backlight control application.
     */
    String packageName;

    /**
     * The source of the ambient backlight.
     */
    AmbientBacklightSource source;

    /**
     * The maximum framerate for the ambient backlight.
     */
    int maxFramerate;

    /**
     * The color format for the ambient backlight.
     */
    PixelFormat colorFormat;

    /**
     * The number of zones in horizontal direction.
     */
    int hZonesNumber;

    /**
     * The number of zones in vertical direction.
     */
    int vZonesNumber;

    /**
     * When a video has a different aspect ratio than the display people
     * watching it on, they often get black bars at the top and bottom
     * (or sometimes the sides). These black bars are called "letterboxing".
     * It's a way to show the entire video without distortion, but it means
     * some of the screen space is unused. This configuration determines
     * whether to ignore the black bar used for padding.
     */
    boolean hasLetterbox;

    /**
     * The color threshold for the ambient backlight. The units of the color deopends on
     * the colorFormat. For example, RGB888, where the values of R/G/B range from 0 to 255,
     * and the threshold is a positive number within the same range.
     */
    int threshold;
}
