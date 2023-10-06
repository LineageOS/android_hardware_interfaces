/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.broadcastradio;

/**
 * Defines the AM/FM band range for configuring different regions.
 *
 * <p>Channel grid is defined as: each possible channel is set at
 * lowerBound + channelNumber * spacing, up to upperBound.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable AmFmBandRange {
    /**
     * The frequency (in kHz) of the first channel within the range.
     *
     * Lower bound must be a tunable frequency.
     */
    int lowerBound;

    /**
     * The frequency (in kHz) of the last channel within the range.
     */
    int upperBound;

    /**
     * Channel grid resolution (in kHz), telling how far the channels are apart.
     */
    int spacing;

    /**
     * Channel spacing (in kHz) used to speed up seeking to the next station
     * via the {@link IBroadcastRadio#seek} operation.
     *
     * It must be a multiple of channel grid resolution.
     *
     * Tuner may first quickly check every n-th channel and if it detects echo
     * from a station, it fine-tunes to find the exact frequency.
     *
     * It's ignored for capabilities check (with full=true when calling
     * getAmFmRegionConfig).
     */
    int seekSpacing;
}
