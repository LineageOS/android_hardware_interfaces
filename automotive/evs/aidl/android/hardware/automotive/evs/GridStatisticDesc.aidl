/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.automotive.evs;

import android.hardware.automotive.evs.ColorChannel;
import android.hardware.automotive.evs.GridStatisticType;
import android.hardware.automotive.evs.Size;
import android.hardware.graphics.common.Rect;

/**
 * This data structure describes a grid statistic such as the low resolution luminance map.
 */
@VintfStability
parcelable GridStatisticDesc {
    /** Source color channel this statistics is calculated from. */
    ColorChannel channel;
    /** Type of this grad statistics. */
    GridStatisticType type;
    /** Region this statistics is calculated from. */
    Rect roi;
    /** Size of a grid cell. */
    Size cellSize;
    /** Bit-depth of a single value. */
    int bitDepth;
}
