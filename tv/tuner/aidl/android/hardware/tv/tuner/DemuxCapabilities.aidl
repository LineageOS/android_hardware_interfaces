/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

/**
 * Capabilities for Demux.
 * @hide
 */
@VintfStability
parcelable DemuxCapabilities {
    /**
     * The number of Demux to be supported.
     */
    int numDemux;

    /**
     * The number of record to be supported.
     */
    int numRecord;

    /**
     * The number of playback to be supported.
     */
    int numPlayback;

    /**
     * The number of TS Filter to be supported.
     */
    int numTsFilter;

    /**
     * The number of Section Filter to be supported.
     */
    int numSectionFilter;

    /**
     * The number of Audio Filter to be supported.
     */
    int numAudioFilter;

    /**
     * The number of Video Filter to be supported.
     */
    int numVideoFilter;

    /**
     * The number of PES Filter to be supported.
     */
    int numPesFilter;

    /**
     * The number of PCR Filter to be supported.
     */
    int numPcrFilter;

    /**
     * The maximum number of bytes is supported in the mask of Section Filter.
     */
    long numBytesInSectionFilter;

    /**
     * Filter Main Types defined by DemuxFilterMainType. The DemuxFilterMainTypes
     * is set by bitwise OR.
     */
    int filterCaps;

    /**
     * The array has same elements as DemuxFilterMainType. linkCaps[i] presents
     * filter's capability as source for the ith type in DemuxFilterMainType.
     * The jth bit of linkCaps[i] is 1 if the output of ith type filter can be
     * data source for the filter type j.
     */
    int[] linkCaps;

    /**
     * True if Time Filter to be supported.
     */
    boolean bTimeFilter;
}
