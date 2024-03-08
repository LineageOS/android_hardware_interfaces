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

import android.hardware.bluetooth.ranging.Config;
import android.hardware.bluetooth.ranging.DeviceAddress;
import android.hardware.bluetooth.ranging.LocationType;
import android.hardware.bluetooth.ranging.Role;
import android.hardware.bluetooth.ranging.SessionType;
import android.hardware.bluetooth.ranging.SightType;
import android.hardware.bluetooth.ranging.VendorSpecificData;

/**
 * Parameters for IBluetoothChannelSoundingSession.openSession().
 */
@VintfStability
parcelable BluetoothChannelSoundingParameters {
    SessionType sessionType;
    /**
     * Acl handle of the connection.
     */
    int aclHandle;
    /**
     * L2CAP Cid, needed in case of EATT which may use dynamic channel for GATT.
     */
    int l2capCid;
    /**
     * ATT handle of the Real-time Procedure Data.
     */
    int realTimeProcedureDataAttHandle;
    /**
     * Role of the local device.
     */
    Role role;
    /**
     * If sounding phase-based ranging is supported by the local device.
     */
    boolean localSupportsSoundingPhaseBasedRanging;
    /**
     * If sounding phase-based ranging is supported by the remote device.
     */
    boolean remoteSupportsSoundingPhaseBaseRanging;
    /**
     * CS conifg used for procedure enable.
     */
    Config config;
    /**
     * Device address of the remote device.
     */
    DeviceAddress address;
    /**
     * Vendor-specific data get from remote GATT Server
     */
    @nullable VendorSpecificData[] vendorSpecificData;
    /**
     * Specifies the preferred location type of the use case (indoor, outdoor, unknown), this is
     * used by the HAL to choose the corresponding ranging algorithm if it supports multiple
     * algorithms
     */
    LocationType locationType;
    /**
     * Specifies the preferred sight type of the use case (line-of-sight, non-line-of-sight,
     * unknown), this is used by the HAL to choose the corresponding ranging algorithm if it
     * supports multiple algorithms
     */
    SightType sightType;
}
