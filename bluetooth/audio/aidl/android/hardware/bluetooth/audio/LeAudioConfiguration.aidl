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
import android.hardware.bluetooth.audio.LeAudioAseConfiguration;
import android.hardware.bluetooth.audio.LeAudioCodecConfiguration;

@VintfStability
parcelable LeAudioConfiguration {
    @VintfStability
    parcelable StreamMap {
        /*
         * The connection handle used for a unicast group.
         * Range: 0x0000 to 0xEFFF
         */
        char streamHandle;
        /*
         * Audio channel allocation is a bit field, each enabled bit means that
         * given audio direction, i.e. "left", or "right" is used. Ordering of
         * audio channels comes from the least significant bit to the most
         * significant bit. The valus follows the Bluetooth SIG Audio Location
         * assigned number.
         */
        int audioChannelAllocation;
        /*
         * The stream handle status
         */
        boolean isStreamActive;
        /*
         * LE Audio device ASE configuration
         */
        @nullable LeAudioAseConfiguration aseConfiguration;
        /*
         * Additional flags, used for configurations with special features
         */
        @nullable ConfigurationFlags flags;
        parcelable BluetoothDeviceAddress {
            enum DeviceAddressType {
                BLE_ADDRESS_PUBLIC = 0x00,
                BLE_ADDRESS_RANDOM = 0x01,
            }
            /**
             * Peer device address. It should be non zero when isStreamActive is true
             */
            byte[6] deviceAddress;
            /**
             * Peer device address type.
             */
            DeviceAddressType deviceAddressType;
        }
        @nullable BluetoothDeviceAddress bluetoothDeviceAddress;
    }
    CodecType codecType;
    StreamMap[] streamMap;
    int peerDelayUs;
    LeAudioCodecConfiguration leAudioCodecConfig;

    /*
     * Bluetooth LTV format for vendor metadata is defined in the
     * Section 6.12.6.9 Vendor_Specific of Bluetooth Assigned Numbers
     *
     * Octet 0 = Length
     * Octet 1 = Type (Vendor specific - 0xFF)
     * Octet 2-3 = Company_ID
     * Company ID values are defined in Bluetooth Assigned Numbers.
     * Octet 4 onwards = Vendor specific Metadata
     */
    @nullable byte[] vendorSpecificMetadata;
}
