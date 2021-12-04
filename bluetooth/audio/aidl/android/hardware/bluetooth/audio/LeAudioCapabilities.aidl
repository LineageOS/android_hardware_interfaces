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

import android.hardware.bluetooth.audio.AudioLocation;
import android.hardware.bluetooth.audio.CodecType;
import android.hardware.bluetooth.audio.Lc3Capabilities;
import android.hardware.bluetooth.audio.LeAudioMode;

/**
 * Used to specify the capabilities of the LC3 codecs supported by Hardware Encoding.
 */
@VintfStability
parcelable LeAudioCapabilities {
    @VintfStability
    parcelable VendorCapabilities {
        ParcelableHolder extension;
    }
    @VintfStability
    union LeaudioCodecCapabilities {
        Lc3Capabilities lc3Capabilities;
        VendorCapabilities vendorCapabillities;
    }
    LeAudioMode mode;
    CodecType codecType;
    /*
     * This is bitfield, if bit N is set, HW Offloader supports N+1 channels at the same time.
     * Example: 0x27 = 0b00100111: One, two, three or six channels supported.
     */
    AudioLocation supportedChannel;
    int supportedChannelCount;
    LeaudioCodecCapabilities leaudioCodecCapabilities;
}
