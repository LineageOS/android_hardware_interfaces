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

import android.hardware.radio.BarringInfo;
import android.hardware.radio.CdmaCallWaiting;
import android.hardware.radio.CdmaInformationRecords;
import android.hardware.radio.CdmaOtaProvisionStatus;
import android.hardware.radio.CdmaSignalInfoRecord;
import android.hardware.radio.CdmaSmsMessage;
import android.hardware.radio.CdmaSubscriptionSource;
import android.hardware.radio.CellIdentity;
import android.hardware.radio.CellInfo;
import android.hardware.radio.Domain;
import android.hardware.radio.EmergencyNumber;
import android.hardware.radio.HardwareConfig;
import android.hardware.radio.KeepaliveStatus;
import android.hardware.radio.LceDataInfo;
import android.hardware.radio.LinkCapacityEstimate;
import android.hardware.radio.NetworkScanResult;
import android.hardware.radio.PbReceivedStatus;
import android.hardware.radio.PcoDataInfo;
import android.hardware.radio.PhoneRestrictedState;
import android.hardware.radio.PhonebookRecordInfo;
import android.hardware.radio.PhysicalChannelConfig;
import android.hardware.radio.RadioCapability;
import android.hardware.radio.RadioIndicationType;
import android.hardware.radio.RadioState;
import android.hardware.radio.RadioTechnology;
import android.hardware.radio.SetupDataCallResult;
import android.hardware.radio.SignalStrength;
import android.hardware.radio.SimRefreshResult;
import android.hardware.radio.SrvccState;
import android.hardware.radio.StkCcUnsolSsResult;
import android.hardware.radio.SuppSvcNotification;
import android.hardware.radio.UssdModeType;

/**
 * Interface declaring unsolicited radio indications.
 */
@VintfStability
interface IRadioIndication {
    /**
     * Indicate barring information for the user’s access category / access class and PLMN.
     *
     * <p>Provide information about the barring status of the cell for the user. The information
     * provided should describe all barring configurations that are applicable to the current user,
     * even if the user is not currently barred (due to conditional barring). This informs Android
     * of likely future (statistical) barring for specific services.
     *
     * <p>This indication should be sent whenever the cell’s barring config changes for the current
     * user, or if the user’s conditional barring status changes due to re-evaluation of the
     * barring conditions. Barring status will likely change when the device camps for service,
     * when PLMN selection is completed, when the device attempts to access a conditionally barred
     * service, and when the System Information including barring info for a camped cell is updated.
     */
    oneway void barringInfoChanged(in RadioIndicationType type, in CellIdentity cellIdentity,
            in BarringInfo[] barringInfos);

    /**
     * Ring indication for an incoming call (eg, RING or CRING event). There must be at least one
     * callRing() at the beginning of a call and sending multiple is optional. If the system
     * property ro.telephony.call_ring.multiple is false then the upper layers must generate the
     * multiple events internally. Otherwise the vendor code must generate multiple callRing() if
     * ro.telephony.call_ring.multiple is true or if it is absent.
     * The rate of these events is controlled by ro.telephony.call_ring.delay and has a default
     * value of 3000 (3 seconds) if absent.
     *
     * @param type Type of radio indication
     * @param isGsm true for GSM & false for CDMA
     * @param record Cdma Signal Information
     */
    oneway void callRing(
            in RadioIndicationType type, in boolean isGsm, in CdmaSignalInfoRecord record);

    /**
     * Indicates when call state has changed. Callee must invoke IRadio.getCurrentCalls(). Must be
     * invoked on, for example, "RING", "BUSY", "NO CARRIER", and also call state transitions
     * (DIALING->ALERTING ALERTING->ACTIVE). Redundent or extraneous invocations are tolerated.
     *
     * @param type Type of radio indication
     */
    oneway void callStateChanged(in RadioIndicationType type);

    /**
     * Indicates that the modem requires the Carrier info for IMSI/IMPI encryption. This might
     * happen when the modem restarts or for some reason it's cache has been invalidated.
     *
     * @param type Type of radio indication
     */
    oneway void carrierInfoForImsiEncryption(in RadioIndicationType info);

    /**
     * Indicates when CDMA radio receives a call waiting indication.
     *
     * @param type Type of radio indication
     * @param callWaitingRecord Cdma CallWaiting information
     */
    oneway void cdmaCallWaiting(in RadioIndicationType type, in CdmaCallWaiting callWaitingRecord);

    /**
     * Indicates when CDMA radio receives one or more info recs.
     *
     * @param type Type of radio indication
     * @param records New Cdma Information
     */
    oneway void cdmaInfoRec(in RadioIndicationType type, in CdmaInformationRecords records);

    /**
     * Indicates when new CDMA SMS is received. Callee must subsequently confirm the receipt of the
     * SMS with acknowledgeLastIncomingCdmaSms(). Server must not send cdmaNewSms() messages until
     * acknowledgeLastIncomingCdmaSms() has been received.
     *
     * @param type Type of radio indication
     * @param msg Cdma Sms Message
     */
    oneway void cdmaNewSms(in RadioIndicationType type, in CdmaSmsMessage msg);

    /**
     * Indicates when CDMA radio receives an update of the progress of an OTASP/OTAPA call.
     *
     * @param type Type of radio indication
     * @param status Cdma OTA provision status
     */
    oneway void cdmaOtaProvisionStatus(
            in RadioIndicationType type, in CdmaOtaProvisionStatus status);

    /**
     * Indicates when PRL (preferred roaming list) changes.
     *
     * @param type Type of radio indication
     * @param version PRL version after PRL changes
     */
    oneway void cdmaPrlChanged(in RadioIndicationType type, in int version);

    /**
     * Indicates that SMS storage on the RUIM is full. Messages cannot be saved on the RUIM until
     * space is freed.
     *
     * @param type Type of radio indication
     */
    oneway void cdmaRuimSmsStorageFull(in RadioIndicationType type);

    /**
     * Indicates when CDMA subscription source changed.
     *
     * @param type Type of radio indication
     * @param cdmaSource New Cdma SubscriptionSource
     */
    oneway void cdmaSubscriptionSourceChanged(
            in RadioIndicationType type, in CdmaSubscriptionSource cdmaSource);

    /**
     * Report all of the current cell information known to the radio.
     *
     * @param type Type of radio indication
     * @param records Current cell information
     */
    oneway void cellInfoList(in RadioIndicationType type, in CellInfo[] records);

    /**
     * Report the current list of emergency numbers. Each emergency number in the emergency number
     * list contains a dialing number, zero or more service category(s), zero or more emergency
     * uniform resource names, mobile country code, mobile network code, and source(s) that indicate
     * where it comes from.
     * Radio must report all the valid emergency numbers with known mobile country code, mobile
     * network code, emergency service categories, and emergency uniform resource names from all
     * available sources including network signaling, sim, modem/oem configuration, and default
     * configuration (112 and 911 must be always available; additionally, 000, 08, 110, 999, 118
     * and 119 must be available when sim is not present). Radio shall not report emergency numbers
     * that are invalid in the current locale. The reported emergency number list must not have
     * duplicate EmergencyNumber entries. Please refer the documentation of EmergencyNumber to
     * construct each emergency number to report.
     * Radio must report the complete list of emergency numbers whenever the emergency numbers in
     * the list are changed or whenever the client and the radio server are connected.
     *
     * Reference: 3gpp 22.101, Section 10 - Emergency Calls;
     *            3gpp 24.008, Section 9.2.13.4 - Emergency Number List
     *
     * @param type Type of radio indication
     * @param emergencyNumberList Current list of emergency numbers known to radio.
     */
    oneway void currentEmergencyNumberList(
            in RadioIndicationType type, in EmergencyNumber[] emergencyNumberList);

    /**
     * Indicates current link capacity estimate. This indication is sent whenever the reporting
     * criteria, as set by IRadio.setLinkCapacityReportingCriteria, are met and the indication is
     * not suppressed by IRadio.setIndicationFilter().
     *
     * @param type Type of radio indication
     * @param lce LinkCapacityEstimate
     */
    oneway void currentLinkCapacityEstimate(
            in RadioIndicationType type, in LinkCapacityEstimate lce);

    /**
     * Indicates physical channel configurations. An empty configs list shall be returned when the
     * radio is in idle mode (i.e. RRC idle).
     *
     * @param type Type of radio indication
     * @param configs Vector of PhysicalChannelConfigs
     */
    oneway void currentPhysicalChannelConfigs(
            in RadioIndicationType type, in PhysicalChannelConfig[] configs);

    /**
     * Indicates current signal strength of the radio.
     *
     * @param type Type of radio indication
     * @param signalStrength SignalStrength information
     */
    oneway void currentSignalStrength(
            in RadioIndicationType type, in SignalStrength signalStrength);

    /**
     * Indicates data call contexts have changed.
     *
     * @param type Type of radio indication
     * @param dcList Array of SetupDataCallResult identical to that returned by
     *        IRadio.getDataCallList(). It is the complete list of current data contexts including
     *        new contexts that have been activated. A data call is only removed from this list
     *        when any of the below conditions is matched:
     *        - The framework sends a IRadio.deactivateDataCall().
     *        - The radio is powered off/on.
     *        - Unsolicited disconnect from either modem or network side.
     */
    oneway void dataCallListChanged(in RadioIndicationType type, in SetupDataCallResult[] dcList);

    /**
     * Indicates that the radio system selection module has autonomously entered emergency
     * callback mode.
     *
     * @param type Type of radio indication
     */
    oneway void enterEmergencyCallbackMode(in RadioIndicationType type);

    /**
     * Indicates when Emergency Callback Mode Ends. Indicates that the radio system selection module
     * has proactively exited emergency callback mode.
     *
     * @param type Type of radio indication
     */
    oneway void exitEmergencyCallbackMode(in RadioIndicationType type);

    /**
     * Indicates when the hardware configuration associated with the RILd changes.
     *
     * @param type Type of radio indication
     * @param configs Array of hardware configs
     */
    oneway void hardwareConfigChanged(in RadioIndicationType type, in HardwareConfig[] configs);

    /**
     * Indicates when IMS registration state has changed. To get IMS registration state and IMS SMS
     * format, callee needs to invoke getImsRegistrationState().
     *
     * @param type Type of radio indication
     */
    oneway void imsNetworkStateChanged(in RadioIndicationType type);

    /**
     * Indicates that nework doesn't have in-band information, need to play out-band tone.
     *
     * @param type Type of radio indication
     * @param start true = start play ringback tone, false = stop playing ringback tone
     */
    oneway void indicateRingbackTone(in RadioIndicationType type, in boolean start);

    /**
     * Indicates a status update for a particular Keepalive session. This must include a handle for
     * a previous session and should include a status update regarding the state of a keepalive.
     * Unsolicited keepalive status reports should never be PENDING as unsolicited status should
     * only be sent when known.
     *
     * @param type Type of radio indication
     * @param status Status information for a Keepalive session
     */
    oneway void keepaliveStatus(in RadioIndicationType type, in KeepaliveStatus status);

    /**
     * Indicates when there is an incoming Link Capacity Estimate (LCE) info report.
     *
     * @param type Type of radio indication
     * @param lce LceData information
     *
     * DEPRECATED in @1.2 and above, use IRadioIndication.currentLinkCapacityEstimate() instead.
     */
    oneway void lceData(in RadioIndicationType type, in LceDataInfo lce);

    /**
     * Indicates when there is a modem reset.
     * When modem restarts, one of the following radio state transitions must happen
     * 1) RadioState:ON->RadioState:UNAVAILABLE->RadioState:ON or
     * 2) RadioState:OFF->RadioState:UNAVAILABLE->RadioState:OFF
     * This message must be sent either just before the Radio State changes to
     * RadioState:UNAVAILABLE or just after but must never be sent after the Radio State changes
     * from RadioState:UNAVAILABLE to RadioState:ON/RadioState:OFF again. It must NOT be sent after
     * the Radio state changes to RadioState:ON/RadioState:OFF after the modem restart as that may
     * be interpreted as a second modem reset by the framework.
     *
     * @param type Type of radio indication
     * @param reason the reason for the reset. It may be a crash signature if the restart was due to
     *        a crash or some string such as "user-initiated restart" or "AT command initiated
     *        restart" that explains the cause of the modem restart
     */
    oneway void modemReset(in RadioIndicationType type, in String reason);

    /**
     * Incremental network scan results.
     *
     * @param type Type of radio indication
     * @param result the result of the network scan
     */
    oneway void networkScanResult(in RadioIndicationType type, in NetworkScanResult result);

    /**
     * Indicates when voice or data network state changed. Callee must invoke
     * IRadio.getVoiceRegistrationState(), IRadio.getDataRegistrationState(), and
     * IRadio.getOperator()
     *
     * @param type Type of radio indication
     */
    oneway void networkStateChanged(in RadioIndicationType type);

    /**
     * Indicates when new Broadcast SMS is received
     *
     * @param type Type of radio indication
     * @param data If received from GSM network, "data" is byte array of 88 bytes which indicates
     *        each page of a CBS Message sent to the MS by the BTS as coded in 3GPP 23.041 Section
     *        9.4.1.2. If received from UMTS network, "data" is byte array of 90 up to 1252 bytes
     *        which contain between 1 and 15 CBS Message pages sent as one packet to the MS by the
     *        BTS as coded in 3GPP 23.041 Section 9.4.2.2
     */
    oneway void newBroadcastSms(in RadioIndicationType type, in byte[] data);

    /**
     * Indicates when new SMS is received. Callee must subsequently confirm the receipt of the SMS
     * with a acknowledgeLastIncomingGsmSms(). Server must not send newSms() or newSmsStatusReport()
     * messages until an acknowledgeLastIncomingGsmSms() has been received.
     *
     * @param type Type of radio indication
     * @param pdu PDU of SMS-DELIVER represented as byte array.
     *        The PDU starts with the SMSC address per TS 27.005 (+CMT:)
     */
    oneway void newSms(in RadioIndicationType type, in byte[] pdu);

    /**
     * Indicates when new SMS has been stored on SIM card
     *
     * @param type Type of radio indication
     * @param recordNumber Record number on the sim
     */
    oneway void newSmsOnSim(in RadioIndicationType type, in int recordNumber);

    /**
     * Indicates when new SMS Status Report is received. Callee must subsequently confirm the
     * receipt of the SMS with a acknowledgeLastIncomingGsmSms(). Server must not send newSms() or
     * newSmsStatusReport() messages until an acknowledgeLastIncomingGsmSms() has been received
     *
     * @param type Type of radio indication
     * @param pdu PDU of SMS-STATUS-REPORT represented as byte array.
     *        The PDU starts with the SMSC address per TS 27.005 (+CMT:)
     */
    oneway void newSmsStatusReport(in RadioIndicationType type, in byte[] pdu);

    /**
     * Indicates when radio has received a NITZ time message.
     *
     * @param type Type of radio indication
     * @param nitzTime NITZ time string in the form "yy/mm/dd,hh:mm:ss(+/-)tz,dt"
     * @param receivedTime milliseconds since boot that the NITZ time was received
     */
    oneway void nitzTimeReceived(
            in RadioIndicationType type, in String nitzTime, in long receivedTime);

    /**
     * Indicates when Supplementary service(SS) response is received when DIAL/USSD/SS is changed to
     * SS by call control.
     *
     * @param type Type of radio indication
     */
    oneway void onSupplementaryServiceIndication(
            in RadioIndicationType type, in StkCcUnsolSsResult ss);

    /**
     * Indicates when a new USSD message is received. The USSD session is assumed to persist if the
     * type code is REQUEST, otherwise the current session (if any) is assumed to have terminated.
     *
     * @param type Type of radio indication
     * @param modeType USSD type code
     * @param msg Message string in UTF-8, if applicable
     */
    oneway void onUssd(in RadioIndicationType type, in UssdModeType modeType, in String msg);

    /**
     * Indicates when there is new Carrier PCO data received for a data call. Ideally only new data
     * must be forwarded, though this is not required. Multiple boxes of carrier PCO data for a
     * given call must result in a series of pcoData() calls.
     *
     * @param type Type of radio indication
     * @param pco New PcoData
     */
    oneway void pcoData(in RadioIndicationType type, in PcoDataInfo pco);

    /**
     * Sent when setRadioCapability() completes. Returns the phone radio capability exactly as
     * getRadioCapability() and must be the same set as sent by setRadioCapability().
     *
     * @param type Type of radio indication
     * @param rc Current radio capability
     */
    oneway void radioCapabilityIndication(in RadioIndicationType type, in RadioCapability rc);

    /**
     * Indicates when radio state changes.
     *
     * @param type Type of radio indication
     * @param radioState Current radio state
     */
    oneway void radioStateChanged(in RadioIndicationType type, in RadioState radioState);

    /**
     * Report that Registration or a Location/Routing/Tracking Area update has failed.
     *
     * <p>Indicate whenever a registration procedure, including a location, routing, or tracking
     * area update fails. This includes procedures that do not necessarily result in a change of
     * the modem's registration status. If the modem's registration status changes, that is
     * reflected in the onNetworkStateChanged() and subsequent get{Voice/Data}RegistrationState().
     *
     * @param cellIdentity the CellIdentity, which must include the globally unique identifier for
     *        the cell (for example, all components of the CGI or ECGI).
     * @param chosenPlmn a 5 or 6 digit alphanumeric PLMN (MCC|MNC) among those broadcast by the
     *        cell that was chosen for the failed registration attempt.
     * @param domain Domain::CS, Domain::PS, or both in case of a combined procedure.
     * @param causeCode the primary failure cause code of the procedure.
     *        For GSM/UMTS (MM), values are in TS 24.008 Sec 10.5.95
     *        For GSM/UMTS (GMM), values are in TS 24.008 Sec 10.5.147
     *        For LTE (EMM), cause codes are TS 24.301 Sec 9.9.3.9
     *        For NR (5GMM), cause codes are TS 24.501 Sec 9.11.3.2
     *        MAX_INT if this value is unused.
     * @param additionalCauseCode the cause code of any secondary/combined procedure if appropriate.
     *        For UMTS, if a combined attach succeeds for PS only, then the GMM cause code shall be
     *        included as an additionalCauseCode.
     *        For LTE (ESM), cause codes are in TS 24.301 9.9.4.4
     *        MAX_INT if this value is unused.
     */
    oneway void registrationFailed(in RadioIndicationType type, in CellIdentity cellIdentity,
            in String chosenPlmn, in Domain domain, in int causeCode, in int additionalCauseCode);

    /**
     * Indicates that framework/application must reset the uplink mute state.
     *
     * @param type Type of radio indication
     */
    oneway void resendIncallMute(in RadioIndicationType type);

    /**
     * Indicates a restricted state change (eg, for Domain Specific Access Control).
     * Radio must send this msg after radio off/on cycle no matter it is changed or not.
     *
     * @param type Type of radio indication
     * @param state Bitmask of restricted state as defined by PhoneRestrictedState
     */
    oneway void restrictedStateChanged(in RadioIndicationType type, in PhoneRestrictedState state);

    /**
     * Indicates the ril connects and returns the version
     *
     * @param type Type of radio indication
     */
    oneway void rilConnected(in RadioIndicationType type);

    /**
     * Indicates whether SIM phonebook is changed. This indication is sent whenever the SIM
     * phonebook is changed, including SIM is inserted or removed and updated by
     * IRadio.updateSimPhonebookRecords.
     *
     * @param type Type of radio indication
     */
    oneway void simPhonebookChanged(in RadioIndicationType type);

    /**
     * Indicates the content of all the used records in the SIM phonebook. This indication is
     * associated with the API getSimPhonebookRecords and might be received more than once that is
     * replying on the record count.
     *
     * @param type Type of radio indication
     * @param status Status of PbReceivedStatus
     * @param records Vector of PhonebookRecordInfo
     */
    oneway void simPhonebookRecordsReceived(in RadioIndicationType type, in PbReceivedStatus status,
            in PhonebookRecordInfo[] records);

    /**
     * Indicates that file(s) on the SIM have been updated, or the SIM has been reinitialized.
     * If the SIM state changes as a result of the SIM refresh (eg, SIM_READY ->
     * SIM_LOCKED_OR_ABSENT), simStatusChanged() must be sent.
     *
     * @param type Type of radio indication
     * @param refreshResult Result of sim refresh
     */
    oneway void simRefresh(in RadioIndicationType type, in SimRefreshResult refreshResult);

    /**
     * Indicates that SMS storage on the SIM is full. Sent when the network attempts to deliver a
     * new SMS message. Messages cannot be saved on the SIM until space is freed. In particular,
     * incoming Class 2 messages must not be stored.
     *
     * @param type Type of radio indication
     */
    oneway void simSmsStorageFull(in RadioIndicationType type);

    /**
     * Indicates that SIM state changes. Callee must invoke getIccCardStatus().
     *
     * @param type Type of radio indication
     */
    oneway void simStatusChanged(in RadioIndicationType type);

    /**
     * Indicates when Single Radio Voice Call Continuity (SRVCC) progress state has changed.
     *
     * @param type Type of radio indication
     * @param state New Srvcc State
     */
    oneway void srvccStateNotify(in RadioIndicationType type, in SrvccState state);

    /**
     * Indicates when there is an ALPHA from UICC during Call Control.
     *
     * @param type Type of radio indication
     * @param alpha ALPHA string from UICC in UTF-8 format
     */
    oneway void stkCallControlAlphaNotify(in RadioIndicationType type, in String alpha);

    /**
     * Indicates when SIM wants application to setup a voice call.
     *
     * @param type Type of radio indication
     * @param timeout Timeout value in millisec for setting up voice call
     */
    oneway void stkCallSetup(in RadioIndicationType type, in long timeout);

    /**
     * Indicates when SIM notifies applcations some event happens.
     *
     * @param type Type of radio indication
     * @param cmd SAT/USAT commands or responses sent by ME to SIM or commands handled by ME,
     *        represented as byte array starting with first byte of response data for command tag.
     *        Refer to TS 102.223 section 9.4 for command types
     */
    oneway void stkEventNotify(in RadioIndicationType type, in String cmd);

    /**
     * Indicates when SIM issue a STK proactive command to applications
     *
     * @param type Type of radio indication
     * @param cmd SAT/USAT proactive represented as byte array starting with command tag.
     *        Refer to TS 102.223 section 9.4 for command types
     */
    oneway void stkProactiveCommand(in RadioIndicationType type, in String cmd);

    /**
     * Indicates when STK session is terminated by SIM.
     *
     * @param type Type of radio indication
     */
    oneway void stkSessionEnd(in RadioIndicationType type);

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
    oneway void subscriptionStatusChanged(in RadioIndicationType type, in boolean activate);

    /**
     * Reports supplementary service related notification from the network.
     *
     * @param type Type of radio indication
     * @param suppSvc SuppSvcNotification as defined in types.hal
     */
    oneway void suppSvcNotify(in RadioIndicationType type, in SuppSvcNotification suppSvc);

    /**
     * Report change of whether uiccApplications are enabled, or disabled.
     *
     * @param type Type of radio indication
     * @param enabled whether uiccApplications are enabled, or disabled
     */
    oneway void uiccApplicationsEnablementChanged(in RadioIndicationType type, in boolean enabled);

    /**
     * The modem can explicitly set SetupDataCallResult::suggestedRetryTime after a failure in
     * IRadio.SetupDataCall. During that time, no new calls are allowed to IRadio.SetupDataCall that
     * use the same APN. When IRadioIndication.unthrottleApn is sent, AOSP will no longer throttle
     * calls to IRadio.SetupDataCall for the given APN.
     *
     * @param type Type of radio indication
     * @param apn Apn to unthrottle
     */
    oneway void unthrottleApn(in RadioIndicationType type, in String apn);

    /**
     * Indicates that voice technology has changed. Responds with new rat.
     *
     * @param type Type of radio indication
     * @param rat Current new voice rat
     */
    oneway void voiceRadioTechChanged(in RadioIndicationType type, in RadioTechnology rat);
}
