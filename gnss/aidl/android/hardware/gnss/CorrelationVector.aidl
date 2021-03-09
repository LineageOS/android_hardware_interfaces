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

package android.hardware.gnss;

/**
 * Contains info about the correlation output of incoming GNSS signal and a local copy of
 * its corresponding spreading code at a given frequency offset.
 */
@VintfStability
parcelable CorrelationVector {
    /**
     * Frequency offset from reported pseudorange rate for this Correlation Vector.
     */
    double frequencyOffsetMps;

    /**
     * Space between correlation samples in meters.
     */
    double samplingWidthM;

    /**
     * Offset of the first sampling bin in meters.
     * The following sampling bins are located at positive offsets from this value as follows:
     * samplingStartM, samplingStartM + samplingWidthM, ... , samplingStartM +
     * (magnitude.size-1) * samplingWidthM.
     */
    double samplingStartM;

    /**
     * Normalized correlation magnitude values from -1 to 1, the reported value must be encoded as
     * signed 16 bit integer where 1 is represented by 32767 and -1 is represented by -32768.
     *
     * The length of the array is defined by the GNSS chipset.
     */
    int[] magnitude;
}
