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

package android.hardware.radio.sim;

import android.hardware.radio.RadioIndicationType;
import android.hardware.radio.sim.CdmaSubscriptionSource;
import android.hardware.radio.sim.PbReceivedStatus;
import android.hardware.radio.sim.PhonebookRecordInfo;
import android.hardware.radio.sim.SimRefreshResult;

/**
 * Interface declaring unsolicited radio indications for SIM APIs.
 * @hide
 */
@VintfStability
oneway interface IRadioSimIndication {
    /**
     * Indicates that the modem requires the Carrier info for IMSI/IMPI encryption. This might
     * happen when the modem restarts or for some reason it's cache has been invalidated.
     *
     * @param type Type of radio indication
     */
    void carrierInfoForImsiEncryption(in RadioIndicationType info);

    /**
     * Indicates when CDMA subscription source changed.
     *
     * @param type Type of radio indication
     * @param cdmaSource New CdmaSubscriptionSource
     */
    void cdmaSubscriptionSourceChanged(
            in RadioIndicationType type, in CdmaSubscriptionSource cdmaSource);

    /**
     * Indicates whether SIM phonebook is changed. This indication is sent whenever the SIM
     * phonebook is changed, including SIM is inserted or removed and updated by
     * IRadioSim.updateSimPhonebookRecords.
     *
     * @param type Type of radio indication
     */
    void simPhonebookChanged(in RadioIndicationType type);

    /**
     * Indicates the content of all the used records in the SIM phonebook. This indication is
     * associated with the API getSimPhonebookRecords and might be received more than once that is
     * replying on the record count.
     *
     * @param type Type of radio indication
     * @param status Status of PbReceivedStatus
     * @param records Vector of PhonebookRecordInfo
     */
    void simPhonebookRecordsReceived(in RadioIndicationType type, in PbReceivedStatus status,
            in PhonebookRecordInfo[] records);

    /**
     * Indicates that file(s) on the SIM have been updated, or the SIM has been reinitialized.
     * If the SIM state changes as a result of the SIM refresh (eg, SIM_READY ->
     * SIM_LOCKED_OR_ABSENT), simStatusChanged() must be sent.
     *
     * @param type Type of radio indication
     * @param refreshResult Result of sim refresh
     */
    void simRefresh(in RadioIndicationType type, in SimRefreshResult refreshResult);

    /**
     * Indicates that SIM state changes. Callee must invoke getIccCardStatus().
     *
     * @param type Type of radio indication
     */
    void simStatusChanged(in RadioIndicationType type);

    /**
     * Indicates when SIM notifies applcations some event happens.
     *
     * @param type Type of radio indication
     * @param cmd SAT/USAT commands or responses sent by ME to SIM or commands handled by ME,
     *        represented as byte array starting with first byte of response data for command tag.
     *        Refer to TS 102.223 section 9.4 for command types
     */
    void stkEventNotify(in RadioIndicationType type, in String cmd);

    /**
     * Indicates when SIM issue a STK proactive command to applications.
     *
     * @param type Type of radio indication
     * @param cmd SAT/USAT proactive represented as byte array starting with command tag.
     *        Refer to TS 102.223 section 9.4 for command types
     */
    void stkProactiveCommand(in RadioIndicationType type, in String cmd);

    /**
     * Indicates when STK session is terminated by SIM.
     *
     * @param type Type of radio indication
     */
    void stkSessionEnd(in RadioIndicationType type);

    /**
     * Indicated when there is a change in subscription status.
     * This event must be sent in the following scenarios
     * - subscription readiness at modem, which was selected by telephony layer
     * - when subscription is deactivated by modem due to UICC card removal
     * - when network invalidates the subscription i.e. attach reject due to authentication reject
     *
     * @param type Type of radio indication
     * @param activate false for subscription deactivated, true for subscription activated
     */
    void subscriptionStatusChanged(in RadioIndicationType type, in boolean activate);

    /**
     * Report change of whether uiccApplications are enabled, or disabled.
     *
     * @param type Type of radio indication
     * @param enabled whether uiccApplications are enabled or disabled
     */
    void uiccApplicationsEnablementChanged(in RadioIndicationType type, in boolean enabled);
}
