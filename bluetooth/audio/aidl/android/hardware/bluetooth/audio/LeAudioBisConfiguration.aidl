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

import android.hardware.bluetooth.audio.CodecId;
import android.hardware.bluetooth.audio.CodecSpecificConfigurationLtv;
import android.hardware.bluetooth.audio.MetadataLtv;

/**
 * LE Audio BIS configuration. This will be part of the streaming broadcast
 * audio announcement advertised by the BT stack during the broadcast audio
 * stream to inform the remote devices about the broadcast audio configuration.
 * It will also be passed back to the vendor module as part of the currently
 * active LeAudioBroadcastConfiguration for the encoder setup.
 * As defined in Bluetooth Basic Audio Profile Specification, v.1.0.1,
 * Sec. 3.7.2.2, Table 3.15, Level 3.
 */
@VintfStability
parcelable LeAudioBisConfiguration {
    /**
     * Codec ID
     */
    CodecId codecId;

    /**
     * Codec configuration for BIS or group of BISes represented in the LTV
     * types defined by Bluetooht SIG. Regardless of vendor specific
     * configuration being used or not, this shall contain Bluetooth LTV types
     * describing the common stream parameters, at least
     * CodecSpecificConfigurationLtv.SamplingFrequency and
     * CodecSpecificConfigurationLtv.AudioChannelAllocation.
     * This will also be used to verify the requirements on the known LTV types.
     */
    CodecSpecificConfigurationLtv[] codecConfiguration;

    /**
     * Vendor specific codec configuration.
     * This will not be parsed by the BT stack but will be set as the codec
     * specific configuration for the ongoing audio stream, encoded by the
     * vendor module. The remote device will receive this information when being
     * configured for receiveing a brodcast audio stream. If this is populated,
     * only the `vendorCodecConfiguration` will be used when configuring the
     * remote device, otherwise `codecConfiguration` will be used.
     */
    byte[] vendorCodecConfiguration;

    /**
     * Metadata for the particular BIS or group of BISes. This is optional.
     */
    @nullable MetadataLtv[] metadata;
}
