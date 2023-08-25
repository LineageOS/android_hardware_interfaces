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

import android.hardware.automotive.evs.GridStatisticDesc;
import android.hardware.common.Ashmem;

/**
 * This data type represents grid-type statistics such as
 * the luminance map in low-resolution.
 */
@VintfStability
parcelable GridStatistics {
    /** Descriptors of grid statistics in shared memory. */
    GridStatisticDesc[] descriptors;
    /**
     * Shared memory object containing one or more grid statistics.
     *
     * When multiple statistics exist, they are in the same order with descriptors. For example,
     * if the first descriptor represents a statistic whose size is in VGA resolution (640x480)
     * and bitdepth (or precision) is 8-bit, the offset to the second statistic is 640x480x1-byte
     * = 307200 bytes.
     */
    Ashmem data;
}
