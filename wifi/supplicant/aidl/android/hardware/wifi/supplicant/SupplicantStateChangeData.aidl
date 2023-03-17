/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.supplicant.KeyMgmtMask;
import android.hardware.wifi.supplicant.StaIfaceCallbackState;

/**
 * Supplicant state change related information.
 */
@VintfStability
parcelable SupplicantStateChangeData {
    /**
     * New State of the interface. This must be one of the
     * |StaIfaceCallbackState| values.
     */
    StaIfaceCallbackState newState;
    /**
     * ID of the corresponding network which caused this
     * state change event. This must be invalid (-1) if this
     * event is not specific to a particular network.
     */
    int id;
    /**
     * SSID of the corresponding network which caused this state
     * change event. This must be empty if this event is not specific
     * to a particular network.
     */
    byte[] ssid;
    /**
     * BSSID of the corresponding AP which caused this state
     * change event. This must be zero'ed if this event is not
     * specific to a particular network.
     */
    byte[6] bssid;

    /**
     * Currently used key mgmt mask.
     */
    KeyMgmtMask keyMgmtMask;
    /*
     * Frequency of the connected channel in MHz. This must be zero if this
     * event is not specific to a particular network.
     */
    int frequencyMhz;
    /*
     * Flag to indicate that FILS HLP IEs were included in this association.
     * This flag is valid only for WPA_COMPLETED state change.
     */
    boolean filsHlpSent;
}
