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

import android.hardware.bluetooth.audio.AacCapabilities;
import android.hardware.bluetooth.audio.AptxAdaptiveCapabilities;
import android.hardware.bluetooth.audio.AptxCapabilities;
import android.hardware.bluetooth.audio.CodecType;
import android.hardware.bluetooth.audio.Lc3Capabilities;
import android.hardware.bluetooth.audio.LdacCapabilities;
import android.hardware.bluetooth.audio.OpusCapabilities;
import android.hardware.bluetooth.audio.SbcCapabilities;

/**
 * Used to specify the capabilities of the codecs supported by Hardware Encoding.
 * AptX and AptX-HD both use the AptxCapabilities field.
 */
@VintfStability
parcelable CodecCapabilities {
    @VintfStability
    parcelable VendorCapabilities {
        ParcelableHolder extension;
    }
    @VintfStability
    union Capabilities {
        SbcCapabilities sbcCapabilities;
        AacCapabilities aacCapabilities;
        LdacCapabilities ldacCapabilities;
        AptxCapabilities aptxCapabilities;
        AptxAdaptiveCapabilities aptxAdaptiveCapabilities;
        Lc3Capabilities lc3Capabilities;
        VendorCapabilities vendorCapabilities;
        @nullable OpusCapabilities opusCapabilities;
    }
    CodecType codecType;
    Capabilities capabilities;
}
