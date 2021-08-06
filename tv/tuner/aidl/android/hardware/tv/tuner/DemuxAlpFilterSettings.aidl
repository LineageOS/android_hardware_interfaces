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

import android.hardware.tv.tuner.DemuxAlpFilterSettingsFilterSettings;
import android.hardware.tv.tuner.DemuxAlpLengthType;

/**
 * Filter Settings for a ALP filter.
 * @hide
 */
@VintfStability
parcelable DemuxAlpFilterSettings {
    /**
     * Packet type according to A/330 ATSC3.0.
     * 0: IPv4 packet
     * 2: Compressed IP packet
     * 4: Link layer signaling packet
     * 6: Packet Type Extension
     * 8: MPEG-2 Transport Stream
     */
    int packetType;

    DemuxAlpLengthType lengthType = DemuxAlpLengthType.UNDEFINED;

    DemuxAlpFilterSettingsFilterSettings filterSettings;
}
