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

import android.hardware.radio.sim.CardPowerState;
import android.hardware.radio.sim.CarrierRestrictions;
import android.hardware.radio.sim.CdmaSubscriptionSource;
import android.hardware.radio.sim.IRadioSimIndication;
import android.hardware.radio.sim.IRadioSimResponse;
import android.hardware.radio.sim.IccIo;
import android.hardware.radio.sim.ImsiEncryptionInfo;
import android.hardware.radio.sim.PersoSubstate;
import android.hardware.radio.sim.PhonebookRecordInfo;
import android.hardware.radio.sim.SelectUiccSub;
import android.hardware.radio.sim.SessionInfo;
import android.hardware.radio.sim.SimApdu;
import android.hardware.radio.sim.SimLockMultiSimPolicy;

/**
 * This interface is used by telephony and telecom to talk to cellular radio for SIM APIs.
 * All the functions have minimum one parameter:
 * serial: which corresponds to serial no. of request. Serial numbers must only be memorized for the
 * duration of a method call. If clients provide colliding serials (including passing the same
 * serial to different methods), multiple responses (one for each method call) must still be served.
 * setResponseFunctions must work with IRadioSimResponse and IRadioSimIndication.
 * @hide
 */
@VintfStability
oneway interface IRadioSim {
    /**
     * Whether uiccApplications are enabled or disabled.
     * By default uiccApplications must be enabled, unless enableUiccApplications() with enable
     * being false is called.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioSimResponse.areUiccApplicationsEnabledResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void areUiccApplicationsEnabled(in int serial);

    /**
     * Supplies old ICC PIN2 and new PIN2.
     *
     * @param serial Serial number of request.
     * @param oldPin2 Old pin2 value
     * @param newPin2 New pin2 value
     * @param aid AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     *
     * Response function is IRadioSimResponse.changeIccPin2ForAppResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void changeIccPin2ForApp(in int serial, in String oldPin2, in String newPin2, in String aid);

    /**
     * Supplies old ICC PIN and new PIN.
     *
     * @param serial Serial number of request.
     * @param oldPin Old pin value
     * @param newPin New pin value
     * @param aid AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     *
     * Response function is IRadioSimResponse.changeIccPinForAppResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void changeIccPinForApp(in int serial, in String oldPin, in String newPin, in String aid);

    /**
     * Enable or disable UiccApplications on the SIM. If disabled:
     *  - Modem will not register on any network.
     *  - SIM must be PRESENT, and the IccId of the SIM must still be accessible.
     *  - The corresponding modem stack is still functional, e.g. able to make emergency calls or
     *    do network scan.
     * By default if this API is not called, the uiccApplications must be enabled automatically.
     * It must work for both single SIM and DSDS cases for UX consistency.
     * The preference is per SIM, and must be remembered over power cycle, modem reboot, or SIM
     * insertion / unplug.
     *
     * @param serial Serial number of request.
     * @param enable true if to enable uiccApplications, false to disable.
     *
     * Response function is IRadioSimResponse.enableUiccApplicationsResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void enableUiccApplications(in int serial, in boolean enable);

    /**
     * Get carrier restrictions.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioSimResponse.getAllowedCarriersResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void getAllowedCarriers(in int serial);

    /**
     * Request the device MDN / H_SID / H_NID. The request is only allowed when CDMA subscription
     * is available. When CDMA subscription is changed, application layer must re-issue the request
     * to update the subscription information.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioSimResponse.getCdmaSubscriptionResponse()
     *
     * This is available when android.hardware.telephony.cdma is defined.
     */
    void getCdmaSubscription(in int serial);

    /**
     * Request to query the location where the CDMA subscription shall be retrieved.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioSimResponse.getCdmaSubscriptionSourceResponse()
     *
     * This is available when android.hardware.telephony.cdma is defined.
     */
    void getCdmaSubscriptionSource(in int serial);

    /**
     * Query the status of a facility lock state
     *
     * @param serial Serial number of request.
     * @param facility is the facility string code from TS 27.007 7.4
     *        (eg "AO" for BAOC, "SC" for SIM lock)
     * @param password is the password, or "" if not required
     * @param serviceClass is the TS 27.007 service class bit vector of services to query
     * @param appId is AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     *        This is only applicable in the case of Fixed Dialing Numbers (FDN) requests.
     *
     * Response function is IRadioSimResponse.getFacilityLockForAppResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void getFacilityLockForApp(in int serial, in String facility, in String password,
            in int serviceClass, in String appId);

    /**
     * Requests status of the ICC card
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioSimResponse.getIccCardStatusResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void getIccCardStatus(in int serial);

    /**
     * Get the SIM IMSI. Only valid when radio state is "RADIO_STATE_ON"
     *
     * @param serial Serial number of request.
     * @param aid AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     *
     * Response function is IRadioSimResponse.getImsiForAppResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void getImsiForApp(in int serial, in String aid);

    /**
     * Get the phonebook capacity.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioSimResponse.getSimPhonebookCapacityResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void getSimPhonebookCapacity(in int serial);

    /**
     * Get the local and global phonebook records from the SIM card.
     * This should be called again after a simPhonebookChanged notification is received.
     * The phonebook records are received via IRadioSimIndication.simPhonebookRecordsReceived()
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioSimResponse.getSimPhonebookRecordsResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void getSimPhonebookRecords(in int serial);

    /**
     * Close a previously opened logical channel. This command reflects TS 27.007
     * "close logical channel" operation (+CCHC).
     *
     * @param serial Serial number of request.
     * @param channelId session id of the logical channel (+CCHC).
     *
     * Response function is IRadioSimResponse.iccCloseLogicalChannelResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     *
     * @deprecated use iccCloseLogicalChannelWithSessionInfo instead.
     */
    void iccCloseLogicalChannel(in int serial, in int channelId);

    /**
     * Request ICC I/O operation. This is similar to the TS 27.007 "restricted SIM" operation where
     * it assumes all of the EF selection must be done by the callee. Arguments and responses that
     * are unused for certain values of "command" must be ignored or set to empty string.
     * Note that IccIo has a "PIN2" field which may be empty string, or may specify a PIN2 for
     * operations that require a PIN2 (eg updating FDN records).
     *
     * @param serial Serial number of request.
     * @param iccIo IccIo
     *
     * Response function is IRadioSimResponse.iccIoForAppResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void iccIoForApp(in int serial, in IccIo iccIo);

    /**
     * Open a new logical channel and select the given application. This command
     * reflects TS 27.007 "open logical channel" operation (+CCHO).
     *
     * For MEP-A(Multiple enabled profile), only dedicated port 0 is ISDR selectable.
     * e.g., Port0 - for ISDR access and Port1/Port2 - the currently active ports/subscriptions.
     * Port 0 should be transparent to AP and iccLogicalChannel API should remain the same.
     * Even if the ISDR request comes over port1 or port2, Modem would just internally convert the
     * portID to port0 and add the real port index as the payload of MANAGE_CHANNEL command.
     *
     * @param serial Serial number of request.
     * @param aid AID value, See ETSI 102.221 and 101.220.
     * @param p2 P2 value, described in ISO 7816-4. Ignore if equal to RadioConst:P2_CONSTANT_NO_P2
     *
     * Response function is IRadioSimResponse.iccOpenLogicalChannelResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void iccOpenLogicalChannel(in int serial, in String aid, in int p2);

    /**
     * Request APDU exchange on the basic channel. This command reflects TS 27.007
     * "generic SIM access" operation (+CSIM). The modem must ensure proper function of GSM/CDMA,
     * and filter commands appropriately. It must filter channel management and SELECT by DF
     * name commands. "sessionid" field must be ignored.
     *
     * @param serial Serial number of request.
     * @param message SimApdu to be sent
     *
     * Response function is IRadioSimResponse.iccTransmitApduBasicChannelResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void iccTransmitApduBasicChannel(in int serial, in SimApdu message);

    /**
     * Exchange APDUs with a UICC over a previously opened logical channel. This command reflects
     * TS 27.007 "generic logical channel access" operation (+CGLA). The modem must filter channel
     * management and SELECT by DF name commands.
     *
     * @param serial Serial number of request.
     * @param message SimApdu to be sent
     *
     * Response function is IRadioSimResponse.iccTransmitApduLogicalChannelResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void iccTransmitApduLogicalChannel(in int serial, in SimApdu message);

    /**
     * Indicates that the StkService is running and is ready to receive unsolicited stk commands.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioSimResponse.reportStkServiceIsRunningResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void reportStkServiceIsRunning(in int serial);

    /**
     * Returns the response of SIM Authentication through Radio challenge request.
     *
     * @param serial Serial number of request.
     * @param authContext P2 value of authentication command, see P2 parameter in
     *        3GPP TS 31.102 7.1.2
     * @param authData the challenge string in Base64 format, see 3GPP TS 31.102 7.1.2
     * @param aid AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value
     *
     * Response function is IRadioSimResponse.requestIccSimAuthenticationResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void requestIccSimAuthentication(
            in int serial, in int authContext, in String authData, in String aid);

    /**
     * When response type received from a radio indication or radio response is
     * RadioIndicationType:UNSOLICITED_ACK_EXP or RadioResponseType:SOLICITED_ACK_EXP respectively,
     * acknowledge the receipt of those messages by sending responseAcknowledgement().
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void responseAcknowledgement();

    /**
     * Requests to send a SAT/USAT envelope command to SIM.
     * The SAT/USAT envelope command refers to 3GPP TS 11.14 and 3GPP TS 31.111
     *
     * @param serial Serial number of request.
     * @param contents SAT/USAT command in hexadecimal format string starting with command tag
     *
     * Response function is IRadioSimResponse.sendEnvelopeResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void sendEnvelope(in int serial, in String contents);

    /**
     * Requests to send a SAT/USAT envelope command to SIM. The SAT/USAT envelope command refers to
     * 3GPP TS 11.14 and 3GPP TS 31.111. This request has one difference from sendEnvelope():
     * The SW1 and SW2 status bytes from the UICC response are returned along with the response
     * data, using the same structure as iccIOForApp(). The implementation must perform normal
     * processing of a '91XX' response in SW1/SW2 to retrieve the pending proactive command and
     * send it as an unsolicited response, as sendEnvelope() does.
     *
     * @param serial Serial number of request.
     * @param contents SAT/USAT command in hexadecimal format starting with command tag
     *
     * Response function is IRadioSimResponse.sendEnvelopeWithStatusResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void sendEnvelopeWithStatus(in int serial, in String contents);

    /**
     * Requests to send a terminal response to SIM for a received proactive command
     *
     * @param serial Serial number of request.
     * @param contents SAT/USAT response in hexadecimal format string starting with
     *        first byte of response data
     *
     * Response function is IRadioSimResponse.sendTerminalResponseResponseToSim()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void sendTerminalResponseToSim(in int serial, in String contents);

    /**
     * Set carrier restrictions. Expected modem behavior:
     *  If never receives this command:
     *  - Must allow all carriers
     *  Receives this command:
     *  - Only allow carriers specified in carriers. The restriction persists across power cycles
     *    and FDR. If a present SIM is allowed, modem must not reload the SIM. If a present SIM is
     *    *not* allowed, modem must detach from the registered network and only keep emergency
     *    service, and notify Android SIM refresh reset with new SIM state being
     *    CardState:RESTRICTED. Emergency service must be enabled.
     *
     * @param serial Serial number of request.
     * @param carriers CarrierRestrictions consisting allowed and excluded carriers
     * @param multiSimPolicy Policy to be used for devices with multiple SIMs.
     *
     * Response function is IRadioSimResponse.setAllowedCarriersResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void setAllowedCarriers(in int serial, in CarrierRestrictions carriers,
            in SimLockMultiSimPolicy multiSimPolicy);

    /**
     * Provide Carrier specific information to the modem that must be used to encrypt the IMSI and
     * IMPI. Sent by the framework during boot, carrier switch and everytime the framework receives
     * a new certificate.
     *
     * @param serial Serial number of request.
     * @param imsiEncryptionInfo ImsiEncryptionInfo
     *
     * Response function is IRadioSimResponse.setCarrierInfoForImsiEncryptionResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void setCarrierInfoForImsiEncryption(in int serial, in ImsiEncryptionInfo imsiEncryptionInfo);

    /**
     * Request to set the location where the CDMA subscription shall be retrieved
     *
     * @param serial Serial number of request.
     * @param cdmaSub CdmaSubscriptionSource
     *
     * Response function is IRadioSimResponse.setCdmaSubscriptionSourceResponse()
     *
     * This is available when android.hardware.telephony.cdma is defined.
     */
    void setCdmaSubscriptionSource(in int serial, in CdmaSubscriptionSource cdmaSub);

    /**
     * Enable/disable one facility lock
     *
     * @param serial Serial number of request.
     * @param facility is the facility string code from TS 27.007 7.4 (eg "AO" for BAOC)
     * @param lockState false for "unlock" and true for "lock"
     * @param password is the password
     * @param serviceClass is string representation of decimal TS 27.007 service class bit vector.
     *        Eg, the string "1" means "set this facility for voice services"
     * @param appId is AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     *        This is only applicable in the case of Fixed Dialing Numbers (FDN) requests.
     *
     * Response function is IRadioSimResponse.setFacilityLockForAppResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void setFacilityLockForApp(in int serial, in String facility, in boolean lockState,
            in String password, in int serviceClass, in String appId);

    /**
     * Set response functions for SIM radio requests and indications.
     *
     * @param radioSimResponse Object containing response functions
     * @param radioSimIndication Object containing radio indications
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void setResponseFunctions(
            in IRadioSimResponse radioSimResponse, in IRadioSimIndication radioSimIndication);

    /**
     * Set SIM card power state. Request is used to power off or power on the card. It should not
     * generate a CardState.CARDSTATE_ABSENT indication, since the SIM is still physically inserted.
     * When SIM card is in POWER_UP_PASS_THROUGH, the modem does not send any command to it (for
     * example SELECT of MF, or TERMINAL CAPABILITY), and the SIM card is controlled completely by
     * Telephony sending APDUs directly. The SIM card state must be RIL_CARDSTATE_PRESENT and the
     * number of card apps will be 0. No new error code is generated. Emergency calls are supported
     * in the same way as if the SIM card is absent. Pass-through mode is valid only for the
     * specific card session where it is activated, and normal behavior occurs at the next SIM
     * initialization, unless POWER_UP_PASS_THROUGH is requested again.
     * The device is required to power down the SIM card before it can switch the mode between
     * POWER_UP and POWER_UP_PASS_THROUGH. At device power up, the SIM interface is powered up
     * automatically. Each subsequent request to this method is processed only after the completion
     * of the previous one.
     * When the SIM is in POWER_DOWN, the modem should send an empty vector of AppStatus in
     * CardStatus.applications. If a SIM in the POWER_DOWN state is removed and a new SIM is
     * inserted, the new SIM should be in POWER_UP mode by default. If the device is turned off or
     * restarted while the SIM is in POWER_DOWN, then the SIM should turn on normally in POWER_UP
     * mode when the device turns back on.
     *
     * @param serial Serial number of request
     * @param powerUp POWER_DOWN if powering down the SIM card
     *                POWER_UP if powering up the SIM card
     *                POWER_UP_PASS_THROUGH if powering up the SIM card in pass through mode
     *
     * Response function is IRadioSimResponse.setSimCardPowerResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void setSimCardPower(in int serial, in CardPowerState powerUp);

    /**
     * Selection/de-selection of a subscription from a SIM card
     *
     * @param serial Serial number of request.
     * @param uiccSub SelectUiccSub
     *
     * Response function is IRadioSimResponse.setUiccSubscriptionResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void setUiccSubscription(in int serial, in SelectUiccSub uiccSub);

    /**
     * Supplies ICC PIN2. Only called following operation where SIM_PIN2 was returned as a failure
     * from a previous operation.
     *
     * @param serial Serial number of request.
     * @param pin2 PIN2 value
     * @param aid AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     *
     * Response function is IRadioSimResponse.supplyIccPin2ForAppResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void supplyIccPin2ForApp(in int serial, in String pin2, in String aid);

    /**
     * Supplies ICC PIN. Only called if CardStatus has AppState.PIN state
     *
     * @param serial Serial number of request.
     * @param pin PIN value
     * @param aid AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     *
     * Response function is IRadioSimResponse.supplyIccPinForAppResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void supplyIccPinForApp(in int serial, in String pin, in String aid);

    /**
     * Supplies ICC PUK2 and new PIN2.
     *
     * @param serial Serial number of request.
     * @param puk2 PUK2 value
     * @param pin2 New PIN2 value
     * @param aid AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     *
     * Response function is IRadioSimResponse.supplyIccPuk2ForAppResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void supplyIccPuk2ForApp(in int serial, in String puk2, in String pin2, in String aid);

    /**
     * Supplies ICC PUK and new PIN.
     *
     * @param serial Serial number of request.
     * @param puk PUK value
     * @param pin New PIN value
     * @param aid AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     *
     * Response function is IRadioSimResponse.supplyIccPukForAppResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void supplyIccPukForApp(in int serial, in String puk, in String pin, in String aid);

    /**
     * Request that deactivates one category of device personalization. Device personalization
     * generally binds the device so it can only be used on one carrier or even one carrier subnet
     * (See TS 22.022). When the user has gained the rights to unbind the device (at the end of a
     * contract period or other event), the controlKey will be delivered to either the user for
     * manual entry or to a carrier app on the device for automatic entry.
     *
     * @param serial Serial number of request.
     * @param persoType SIM personalization type.
     * @param controlKey the unlock code for removing persoType personalization from this device
     *
     * Response function is IRadioSimResponse.supplySimDepersonalizationResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void supplySimDepersonalization(
            in int serial, in PersoSubstate persoType, in String controlKey);

    /**
     * Insert, delete or update a phonebook record on the SIM card. If the index of recordInfo is 0,
     * the phonebook record will be added to global or local phonebook, and global phonebook has
     * higher priority than local phonebook. If the fields in the recordInfo are all empty except
     * for the index, the phonebook record specified by the index will be deleted. The indication
     * simPhonebookChanged will be called after every successful call of updateSimPhonebookRecords.
     *
     * @param serial Serial number of request.
     * @param recordInfo Details of the record to insert, delete or update.
     *
     * Response function is IRadioSimResponse.updateSimPhonebookRecordsResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void updateSimPhonebookRecords(in int serial, in PhonebookRecordInfo recordInfo);

    /**
     * Close a previously opened logical channel. This command reflects TS 27.007
     * "close logical channel" operation (+CCHC).
     *
     * Per spec SGP.22 V3.0, ES10 commands needs to be sent over command port of MEP-A. In order
     * to close proper logical channel, should pass information about whether the logical channel
     * was opened for sending ES10 commands or not.
     *
     * @param serial Serial number of request.
     * @param sessionInfo Details of the opened logical channel info like sessionId and isEs10.
     *
     * Response function is IRadioSimResponse.iccCloseLogicalChannelWithSessionInfoResponse()
     *
     * This is available when android.hardware.telephony.subscription is defined.
     */
    void iccCloseLogicalChannelWithSessionInfo(in int serial, in SessionInfo sessionInfo);
}
