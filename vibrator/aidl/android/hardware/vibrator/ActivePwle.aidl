/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.vibrator;

@VintfStability
parcelable ActivePwle {
    /**
     * Amplitude ranging from 0.0 (inclusive) to 1.0 (inclusive)
     * in units of output acceleration amplitude, not voltage amplitude.
     *
     * Values should fall within the range of 0.0 (inclusive) to the maximum defined
     * by the corresponding entries in IVibrator#getBandwidthAmplitudeMap (inclusive).
     * If startFrequency falls between two entries, the value will not exceed the
     * largest amplitude of the two bounding frequencies.
     *
     * 0.0 represents no output acceleration amplitude
     * 1.0 represents maximum output acceleration amplitude
     *     across all supported frequencies
     */
    float startAmplitude;

    /**
     * Absolute frequency point in the units of hertz
     *
     * Values are within the continuous inclusive frequency range defined by
     * IVibrator#getBandwidthAmplitudeMap, and not limited by the
     * IVibrator#getFrequencyResolution.
     */
    float startFrequency;

    /**
     * Amplitude ranging from 0.0 (inclusive) to 1.0 (inclusive)
     * in units of output acceleration amplitude, not voltage amplitude.
     *
     * Values should fall within the range of 0.0 (inclusive) to the maximum defined
     * by the corresponding entries in IVibrator#getBandwidthAmplitudeMap (inclusive).
     * If endFrequency falls between two entries, the value will not exceed the
     * largest amplitude of the two bounding frequencies.
     *
     * 0.0 represents no output acceleration amplitude
     * 1.0 represents maximum output acceleration amplitude
     *     across all supported frequencies
     */
    float endAmplitude;

    /**
     * Absolute frequency point in the units of hertz
     *
     * Values are within the continuous inclusive frequency range defined by
     * IVibrator#getBandwidthAmplitudeMap, and not limited by the
     * IVibrator#getFrequencyResolution.
     */
    float endFrequency;

    /**
     * Total duration from start point to end point in the units of milliseconds
     */
    int duration;
}
