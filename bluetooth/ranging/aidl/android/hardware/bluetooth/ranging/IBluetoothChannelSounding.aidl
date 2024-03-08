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

package android.hardware.bluetooth.ranging;

import android.hardware.bluetooth.ranging.BluetoothChannelSoundingParameters;
import android.hardware.bluetooth.ranging.CsSecurityLevel;
import android.hardware.bluetooth.ranging.IBluetoothChannelSoundingSession;
import android.hardware.bluetooth.ranging.IBluetoothChannelSoundingSessionCallback;
import android.hardware.bluetooth.ranging.Nadm;
import android.hardware.bluetooth.ranging.SessionType;
import android.hardware.bluetooth.ranging.VendorSpecificData;

/**
 * The interface for the Bluetooth stack to get vendor specifc data and open session
 * for channel sounding.
 */
@VintfStability
interface IBluetoothChannelSounding {
    /**
     * API to get vendor-specific data, the Bluetooth stack will provision the GATT server with
     * these vendor-specific UUIDs and data.
     *
     * @return an array of vendor specifc data
     */
    @nullable VendorSpecificData[] getVendorSpecificData();

    /**
     * API to get supported session types of the HAL
     *
     * @return an array of supported session types
     */
    @nullable SessionType[] getSupportedSessionTypes();

    /**
     * API to get max supported security level (0 to 4) of CS for ranging algorithms.
     *
     *  See: https://bluetooth.com/specifications/specs/channel-sounding-cr-pr/
     *
     * @return CsSecurityLevel that indicates max supported security level of CS for ranging
     *         algorithms.
     */
    CsSecurityLevel getMaxSupportedCsSecurityLevel();

    /**
     * API to open session for channel sounding and register the corresponeding callback
     *
     * @return an instance of IBluetoothChannelSoundingSession
     */
    @nullable IBluetoothChannelSoundingSession openSession(
            in BluetoothChannelSoundingParameters params,
            in IBluetoothChannelSoundingSessionCallback callback);
}
