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

package android.hardware.radio;

import android.hardware.radio.CellIdentity;
import android.hardware.radio.RegState;

@VintfStability
parcelable VoiceRegStateResult {
    /**
     * Valid reg states are NOT_REG_MT_NOT_SEARCHING_OP, REG_HOME, NOT_REG_MT_SEARCHING_OP,
     * REG_DENIED, UNKNOWN, REG_ROAMING defined in RegState.
     */
    RegState regState;
    /**
     * Indicates the available voice radio technology, valid values as defined by RadioTechnology.
     */
    int rat;
    /**
     * Concurrent services support indicator, if registered on a CDMA system.
     * false - Concurrent services not supported,
     * true - Concurrent services supported
     */
    boolean cssSupported;
    /**
     * TSB-58 Roaming Indicator if registered on a CDMA or EVDO system or -1 if not.
     * Valid values are 0-255.
     */
    int roamingIndicator;
    /**
     * Indicates whether the current system is in the PRL if registered on a CDMA or EVDO system
     * or -1 if not. 0=not in the PRL, 1=in the PRL
     */
    int systemIsInPrl;
    /**
     * Default Roaming Indicator from the PRL if registered on a CDMA or EVDO system or -1 if not.
     * Valid values are 0-255.
     */
    int defaultRoamingIndicator;
    /**
     * Reason for denial if registration state is REG_DENIED. This is an enumerated reason why
     * registration was denied. See 3GPP TS 24.008, 10.5.3.6 and Annex G.
     * 0 - General
     * 1 - Authentication Failure
     * 2 - IMSI unknown in HLR
     * 3 - Illegal MS
     * 4 - Illegal ME
     * 5 - PLMN not allowed
     * 6 - Location area not allowed
     * 7 - Roaming not allowed
     * 8 - No Suitable Cells in this Location Area
     * 9 - Network failure
     * 10 - Persistent location update reject
     * 11 - PLMN not allowed
     * 12 - Location area not allowed
     * 13 - Roaming not allowed in this Location Area
     * 15 - No Suitable Cells in this Location Area
     * 17 - Network Failure
     * 20 - MAC Failure
     * 21 - Sync Failure
     * 22 - Congestion
     * 23 - GSM Authentication unacceptable
     * 25 - Not Authorized for this CSG
     * 32 - Service option not supported
     * 33 - Requested service option not subscribed
     * 34 - Service option temporarily out of order
     * 38 - Call cannot be identified
     * 48-63 - Retry upon entry into a new cell
     * 95 - Semantically incorrect message
     * 96 - Invalid mandatory information
     * 97 - Message type non-existent or not implemented
     * 98 - Message type not compatible with protocol state
     * 99 - Information element non-existent or not implemented
     * 100 - Conditional IE error
     * 101 - Message not compatible with protocol state
     * 111 - Protocol error, unspecified
     */
    int reasonForDenial;
    CellIdentity cellIdentity;
}
