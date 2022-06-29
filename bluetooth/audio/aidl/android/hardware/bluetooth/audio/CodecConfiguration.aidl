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

import android.hardware.bluetooth.audio.AacConfiguration;
import android.hardware.bluetooth.audio.AptxAdaptiveConfiguration;
import android.hardware.bluetooth.audio.AptxConfiguration;
import android.hardware.bluetooth.audio.CodecType;
import android.hardware.bluetooth.audio.Lc3Configuration;
import android.hardware.bluetooth.audio.LdacConfiguration;
import android.hardware.bluetooth.audio.OpusConfiguration;
import android.hardware.bluetooth.audio.SbcConfiguration;

/**
 * Used to configure a Hardware Encoding session.
 * AptX and AptX-HD both use the AptxConfiguration field.
 */
@VintfStability
parcelable CodecConfiguration {
    @VintfStability
    parcelable VendorConfiguration {
        int vendorId;
        char codecId;
        ParcelableHolder codecConfig;
    }
    @VintfStability
    union CodecSpecific {
        SbcConfiguration sbcConfig;
        AacConfiguration aacConfig;
        LdacConfiguration ldacConfig;
        AptxConfiguration aptxConfig;
        AptxAdaptiveConfiguration aptxAdaptiveConfig;
        Lc3Configuration lc3Config;
        VendorConfiguration vendorConfig;
        @nullable OpusConfiguration opusConfig;
    }
    CodecType codecType;
    /**
     * The encoded audio bitrate in bits / second.
     * 0x00000000 - The audio bitrate is not specified / unused
     * 0x00000001 - 0x00FFFFFF - Encoded audio bitrate in bits/second
     * 0x01000000 - 0xFFFFFFFF - Reserved
     *
     * The HAL needs to support all legal bitrates for the selected codec.
     */
    int encodedAudioBitrate;
    /**
     * Peer MTU (in two-octets)
     */
    int peerMtu;
    /**
     * Content protection by SCMS-T
     */
    boolean isScmstEnabled;
    CodecSpecific config;
}
