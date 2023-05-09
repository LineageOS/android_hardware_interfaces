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

package android.hardware.radio.data;

import android.hardware.radio.RadioIndicationType;
import android.hardware.radio.data.DataProfileInfo;
import android.hardware.radio.data.KeepaliveStatus;
import android.hardware.radio.data.PcoDataInfo;
import android.hardware.radio.data.SetupDataCallResult;
import android.hardware.radio.data.SlicingConfig;

/**
 * Interface declaring unsolicited radio indications for data APIs.
 * @hide
 */
@VintfStability
oneway interface IRadioDataIndication {
    /**
     * Indicates data call contexts have changed.
     *
     * @param type Type of radio indication
     * @param dcList Array of SetupDataCallResult identical to that returned by
     *        IRadioData.getDataCallList(). It is the complete list of current data contexts
     *        including new contexts that have been activated. A data call is only removed from
     *        this list when any of the below conditions is matched:
     *        - The framework sends a IRadioData.deactivateDataCall().
     *        - The radio is powered off/on.
     *        - Unsolicited disconnect from either modem or network side.
     */
    void dataCallListChanged(in RadioIndicationType type, in SetupDataCallResult[] dcList);

    /**
     * Indicates a status update for a particular Keepalive session. This must include a handle for
     * a previous session and should include a status update regarding the state of a keepalive.
     * Unsolicited keepalive status reports should never be PENDING as unsolicited status should
     * only be sent when known.
     *
     * @param type Type of radio indication
     * @param status Status information for a Keepalive session
     */
    void keepaliveStatus(in RadioIndicationType type, in KeepaliveStatus status);

    /**
     * Indicates when there is new Carrier PCO data received for a data call. Ideally only new data
     * must be forwarded, though this is not required. Multiple boxes of carrier PCO data for a
     * given call must result in a series of pcoData() calls.
     *
     * @param type Type of radio indication
     * @param pco New PcoData
     */
    void pcoData(in RadioIndicationType type, in PcoDataInfo pco);

    /**
     * The modem can explicitly set SetupDataCallResult::suggestedRetryTime after a failure in
     * IRadioData.SetupDataCall. During that time, no new calls are allowed to
     * IRadioData.SetupDataCall that use the same APN. When IRadioDataIndication.unthrottleApn
     * is sent, AOSP will no longer throttle calls to IRadioData.SetupDataCall for the given APN.
     *
     * @param type Type of radio indication
     * @param dataProfileInfo Data profile info.
     */
    void unthrottleApn(in RadioIndicationType type, in DataProfileInfo dataProfileInfo);

    /**
     * Indicates the current slicing configuration including URSP rules and NSSAIs
     * (configured, allowed and rejected). URSP stands for UE route selection policy and is defined
     * in 3GPP TS 24.526 Section 4.2. An NSSAI is a collection of network slices. Each network slice
     * is identified by an S-NSSAI and is represented by the struct SliceInfo. NSSAI and S-NSSAI
     * are defined in 3GPP TS 24.501.
     *
     * @param type Type of radio indication
     * @param slicingConfig Current slicing configuration
     *
     */
    void slicingConfigChanged(in RadioIndicationType type, in SlicingConfig slicingConfig);
}
