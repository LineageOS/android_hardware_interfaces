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

package android.hardware.wifi;

import android.hardware.wifi.NanPairingRequestType;
import android.hardware.wifi.NanStatus;
import android.hardware.wifi.NpkSecurityAssociation;
import android.hardware.wifi.common.OuiKeyedData;

/**
 * NAN pairing confirmation indication structure. Event indication is
 * received on both initiator and responder side when negotiation for a
 * pairing finishes on success or failure. See Wi-Fi Aware R4.0 section 7.6.1.4
 */
@VintfStability
parcelable NanPairingConfirmInd {
    /**
     * ID of the pairing session.
     */
    int pairingInstanceId;
    /**
     * Indicates whether the pairing setup succeeded (true) or failed (false).
     */
    boolean pairingSuccess;
    /**
     * Failure reason if |pairingSuccess| is false.
     */
    NanStatus status;
    /**
     * Indicate the pairing session is of setup or verification
     */
    NanPairingRequestType requestType;
    /**
     * Whether should cache the negotiated NIK/NPK for future verification
     */
    boolean enablePairingCache;
    /**
     * The security association negotiated for the pairing, can be cached for future verification
     */
    NpkSecurityAssociation npksa;
    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;
}
