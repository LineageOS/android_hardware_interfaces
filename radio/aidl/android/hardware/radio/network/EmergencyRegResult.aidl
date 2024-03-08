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

package android.hardware.radio.network;
import android.hardware.radio.AccessNetwork;
import android.hardware.radio.network.Domain;
import android.hardware.radio.network.RegState;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable EmergencyRegResult {
    /**
     * Indicates the cellular access network of the current emergency capable system.
     */
    AccessNetwork accessNetwork;

    /**
     * Registration state of the current emergency capable system.
     */
    RegState regState;

    /**
     * EMC domain indicates the current domain of the acquired system.
     */
    Domain emcDomain;

    /**
     * This indicates whether the network supports voice over PS network.
     */
    boolean isVopsSupported;

    /**
     * This indicates if camped network support VoLTE emergency bearers.
     * This should only be set if the UE is in LTE mode.
     */
    boolean isEmcBearerSupported;

    /**
     * The value of the network provided EMC 5G Registration ACCEPT.
     * This should be set only if  the UE is in 5G mode.
     */
    byte nwProvidedEmc;

    /**
     * The value of the network provided EMF ( EPS Fallback) in 5G Registration ACCEPT.
     * This should not be set if UE is not in 5G mode.
     */
    byte nwProvidedEmf;

    /** 3-digit Mobile Country Code, 000..999, empty string if unknown. */
    String mcc = "";

    /** 2 or 3-digit Mobile Network Code, 00..999, empty string if unknown. */
    String mnc = "";
}
