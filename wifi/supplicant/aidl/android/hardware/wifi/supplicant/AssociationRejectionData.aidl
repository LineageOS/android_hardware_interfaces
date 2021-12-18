/*
 * Copyright (C) 2021 The Android Open Source Project
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

import android.hardware.wifi.supplicant.MboAssocDisallowedReasonCode;
import android.hardware.wifi.supplicant.OceRssiBasedAssocRejectAttr;
import android.hardware.wifi.supplicant.StaIfaceStatusCode;

/**
 * Association Rejection related information.
 */
@VintfStability
parcelable AssociationRejectionData {
    /**
     * SSID of the AP that rejected the association.
     */
    byte[] ssid;
    /**
     * BSSID of the AP that rejected the association.
     */
    byte[/* 6 */] bssid;
    /*
     * 802.11 code to indicate the reject reason.
     * Refer to section 8.4.1.9 of IEEE 802.11 spec.
     */
    StaIfaceStatusCode statusCode;
    /*
     * Flag to indicate that failure is due to timeout rather than
     * explicit rejection response from the AP.
     */
    boolean timedOut;
    /**
     * Flag to indicate that MboAssocDisallowedReasonCode is present
     * in the (Re-)Association response frame.
     */
    boolean isMboAssocDisallowedReasonCodePresent;
    /**
     * mboAssocDisallowedReason is extracted from MBO association disallowed attribute
     * in (Re-)Association response frame to indicate that the AP is not accepting new
     * associations.
     * Refer MBO spec v1.2 section 4.2.4 Table 13 for the details of reason code.
     * The value is undefined if isMboAssocDisallowedReasonCodePresent is false.
     */
    MboAssocDisallowedReasonCode mboAssocDisallowedReason;
    /**
     * Flag to indicate that OceRssiBasedAssocRejectAttr is present
     * in the (Re-)Association response frame.
     */
    boolean isOceRssiBasedAssocRejectAttrPresent;
    /*
     * OCE RSSI-based (Re-)Association rejection attribute.
     * The contents are undefined if isOceRssiBasedAssocRejectAttrPresent is false.
     */
    OceRssiBasedAssocRejectAttr oceRssiBasedAssocRejectData;
}
