/*
 * Copyright 2023 The Android Open Source Project
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

import android.hardware.bluetooth.audio.ChannelMode;
import android.hardware.bluetooth.audio.CodecId;
import android.hardware.bluetooth.audio.ConfigurationFlags;

/**
 * General information about a Codec
 */
@VintfStability
parcelable CodecInfo {
    /**
     * Codec identifier and human readable name
     */
    CodecId id;
    String name;

    /**
     * A2DP Context
     */
    parcelable A2dp {
        /**
         * The capabilities as defined by A2DP for codec interoperability
         * requirements. With `id.a2dp`, the format is given by the `Codec
         * Specific Information Elements` [A2DP - 4.3-6.2], and with `id.vendor`,
         * by `Vendor Specific Value` [A2DP - 4.7.2].
         */
        byte[] capabilities;

        /**
         * PCM characteristics:
         * - Mono, Dual-Mono or Stereo
         * - Supported sampling frequencies, in Hz.
         * - Fixed point resolution, basically 16, 24 or 32 bits by samples.
         *   The value 32 should be used for floating point representation.
         *
         * When the bitdepth is not an encoding/decoding parameter (don't take part
         * in the interoperability), the `bitdepth` list shall have a single element
         * indicating the bitdepth selected for the platform.
         */
        ChannelMode[] channelMode;
        int[] samplingFrequencyHz;
        int[] bitdepth;

        /**
         * Lossless capable characteristic
         */
        boolean lossless;
    }

    /**
     * HFP Context
     */
    parcelable Hfp {
        /**
         * Vendor-specific identifiers of stream data paths, set in the
         * HCI Command Enhanced Setup Synchronous Connection [Core - 4.E.7.1.45],
         * in the command parameters respectively `Input_Data_Path` and
         * `Output_Data_Path`. The value range from 0x01 to 0xFE.
         * The stack operates as a pass-through; the client SHALL NOT
         * interpret the values.
         */
        int inputDataPath = 1;
        int outputDataPath = 1;

        /**
         * Whether the audio stream is encoded and decoded in the controller or
         * locally; enable the controller transparent mode when the audio
         * stream is locally processed.
         */
        boolean useControllerCodec = true;
    }

    /**
     * LE Audio Context
     */
    parcelable LeAudio {
        /**
         * Channel configuration: Mono, Dual-Mono or Stereo
         */
        ChannelMode[] channelMode;

        /**
         * Supported sampling frequencies, in Hz.
         */
        int[] samplingFrequencyHz;

        /*
         * FrameDuration in microseconds.
         */
        int[] frameDurationUs;

        /**
         * - Fixed point resolution, basically 16, 24 or 32 bits by samples.
         *   The value 32 should be used for floating point representation.
         *
         * When the bitdepth is not an encoding/decoding parameter (don't take
         * part in the interoperability), the `bitdepth` list shall have a
         * single element indicating the bitdepth selected for the platform.
         */
        int[] bitdepth;

        /**
         * Additional configuration flags
         */
        @nullable ConfigurationFlags flags;
    }

    /**
     * Specific informations,
     * depending on transport.
     */
    union Transport {
        LeAudio leAudio;
        A2dp a2dp;
        Hfp hfp;
    }

    Transport transport;
}
