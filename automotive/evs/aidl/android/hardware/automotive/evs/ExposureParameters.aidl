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

/**
 * Parameters that determine the exposure value.
 *
 * This data structure assumes that the imaging sensor has 4 channels at most.
 */
@VintfStability
parcelable ExposureParameters {
    /**
     * Analog gain applied on each channel.
     *
     * A value for each channel will be indexed by android.hardware.automotive.evs.ColorChannel
     * enum type.
     */
    float[4] analogGain;
    /**
     * Digital gain applied on each channel.
     *
     * A value for each channel will be indexed by android.hardware.automotive.evs.ColorChannel
     * enum type.
     */
    float[4] digitalGain;
    /** Exposure time in lines. */
    long coarseIntegrationTimeInLines;
    /** Remainder exposure time in clocks. */
    long fineIntegrationTimeInPixelClocks;
    /**
     * Logarithm value of coarse integration time multiplier.
     */
    int coarseIntegrationTimeLShift;
}
