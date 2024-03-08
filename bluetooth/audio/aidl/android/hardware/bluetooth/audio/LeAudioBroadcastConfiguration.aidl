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

package android.hardware.bluetooth.audio;

import android.hardware.bluetooth.audio.CodecType;
import android.hardware.bluetooth.audio.ConfigurationFlags;
import android.hardware.bluetooth.audio.LeAudioBisConfiguration;
import android.hardware.bluetooth.audio.LeAudioCodecConfiguration;

@VintfStability
parcelable LeAudioBroadcastConfiguration {
    @VintfStability
    parcelable BroadcastStreamMap {
        /*
         * The connection handle used for a broadcast group.
         * Range: 0x0000 to 0xEFFF
         */
        char streamHandle;
        /*
         * Audio channel allocation is  a bit field, each enabled bit means that given audio
         * direction, i.e. "left", or "right" is used. Ordering of audio channels comes from the
         * least significant bit to the most significant bit.
         */
        int audioChannelAllocation;
        LeAudioCodecConfiguration leAudioCodecConfig;
        /*
         * Pcm stream id to identify the source for given streamHandle.
         */
        char pcmStreamId;
        /*
         * LE Audio BIS configuration
         */
        @nullable LeAudioBisConfiguration bisConfiguration;
        /*
         * Additional flags, used to request configurations with special
         * features
         */
        @nullable ConfigurationFlags flags;
    }
    CodecType codecType;
    BroadcastStreamMap[] streamMap;
}
