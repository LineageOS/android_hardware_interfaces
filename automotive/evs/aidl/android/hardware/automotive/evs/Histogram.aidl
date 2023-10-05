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
import android.hardware.graphics.common.Rect;

/**
 * This data type represents 1D histogram statistics.
 */
@VintfStability
parcelable Histogram {
    /**
     * Source color channel this histogram is calculated from.
     */
    ColorChannel channel;
    /** A region of interests, where this histogram is calculated from. */
    Rect roi;
    /** Size of each histogram bin. */
    int size;
    /** Capacity of each histogram bin. */
    int capacity;
    /**
     * Histogram bins; each bin contains values calculated from each ColorChannel.
     *
     * The size of the histogram data is inversely proportional to the size of a histogram bin.
     * When a bin is sized as 1, a histogram has pow(2, bitdepth of the source data) and
     * therefore could occupy more than a few kilobytes of 1 megabyte buffer that is shared with
     * other processes. To avoid influencing other processes, we strongly recommend keeping the
     * number of histogram bins less than 128. If a higher resolution is needed, please consider
     * using android.hardware.automotive.evs.EmbeddedData instead.
     */
    long[] bins;
}
