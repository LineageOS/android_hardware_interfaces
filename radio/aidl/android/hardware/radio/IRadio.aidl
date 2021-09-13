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

import android.hardware.radio.AccessNetwork;
import android.hardware.radio.CallForwardInfo;
import android.hardware.radio.CardPowerState;
import android.hardware.radio.CarrierRestrictions;
import android.hardware.radio.CdmaBroadcastSmsConfigInfo;
import android.hardware.radio.CdmaRoamingType;
import android.hardware.radio.CdmaSmsAck;
import android.hardware.radio.CdmaSmsMessage;
import android.hardware.radio.CdmaSmsWriteArgs;
import android.hardware.radio.CdmaSubscriptionSource;
import android.hardware.radio.DataProfileInfo;
import android.hardware.radio.DataRequestReason;
import android.hardware.radio.DataThrottlingAction;
import android.hardware.radio.DeviceStateType;
import android.hardware.radio.Dial;
import android.hardware.radio.EmergencyCallRouting;
import android.hardware.radio.EmergencyServiceCategory;
import android.hardware.radio.GsmBroadcastSmsConfigInfo;
import android.hardware.radio.GsmSmsMessage;
import android.hardware.radio.IRadioIndication;
import android.hardware.radio.IRadioResponse;
import android.hardware.radio.IccIo;
import android.hardware.radio.ImsSmsMessage;
import android.hardware.radio.ImsiEncryptionInfo;
import android.hardware.radio.IndicationFilter;
import android.hardware.radio.KeepaliveRequest;
import android.hardware.radio.LinkAddress;
import android.hardware.radio.NetworkScanRequest;
import android.hardware.radio.NrDualConnectivityState;
import android.hardware.radio.NvItem;
import android.hardware.radio.NvWriteItem;
import android.hardware.radio.PersoSubstate;
import android.hardware.radio.PhonebookRecordInfo;
import android.hardware.radio.PreferredNetworkType;
import android.hardware.radio.RadioAccessFamily;
import android.hardware.radio.RadioAccessNetworks;
import android.hardware.radio.RadioAccessSpecifier;
import android.hardware.radio.RadioBandMode;
import android.hardware.radio.RadioCapability;
import android.hardware.radio.RadioTechnology;
import android.hardware.radio.ResetNvType;
import android.hardware.radio.SelectUiccSub;
import android.hardware.radio.SignalThresholdInfo;
import android.hardware.radio.SimApdu;
import android.hardware.radio.SimLockMultiSimPolicy;
import android.hardware.radio.SliceInfo;
import android.hardware.radio.SmsAcknowledgeFailCause;
import android.hardware.radio.SmsWriteArgs;
import android.hardware.radio.TrafficDescriptor;
import android.hardware.radio.TtyMode;

/**
 * This interface is used by telephony and telecom to talk to cellular radio.
 * All the functions have minimum one parameter:
 * serial: which corresponds to serial no. of request. Serial numbers must only be memorized for the
 * duration of a method call. If clients provide colliding serials (including passing the same
 * serial to different methods), multiple responses (one for each method call) must still be served.
 * setResponseFunctions must work with IRadioResponse and IRadioIndication.
 */
@VintfStability
oneway interface IRadio {
    /**
     * Answer incoming call. Must not be called for WAITING calls.
     * switchWaitingOrHoldingAndActive() must be used in this case instead
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.acceptCallResponse()
     */
    void acceptCall(in int serial);

    /**
     * Acknowledge successful or failed receipt of SMS previously indicated via unsol
     * responseNewSms(), including acknowledgement TPDU to send as the RP-User-Data element of the
     * RP-ACK or RP-ERROR PDU.
     *
     * @param serial Serial number of request.
     * @param success true on successful receipt (send RP-ACK)
     *        false on failed receipt (send RP-ERROR)
     * @param ackPdu acknowledgement TPDU in hexadecimal format
     *
     * Response callback is IRadioResponse.acknowledgeIncomingGsmSmsWithPduResponse()
     */
    void acknowledgeIncomingGsmSmsWithPdu(in int serial, in boolean success, in String ackPdu);

    /**
     * Acknowledge the success or failure in the receipt of SMS previously indicated
     * via responseCdmaNewSms()
     *
     * @param serial Serial number of request.
     * @param smsAck Cdma Sms ack to be sent described by CdmaSmsAck in types.hal
     *
     * Response callback is IRadioResponse.acknowledgeLastIncomingCdmaSmsResponse()
     */
    void acknowledgeLastIncomingCdmaSms(in int serial, in CdmaSmsAck smsAck);

    /**
     * Acknowledge successful or failed receipt of SMS previously indicated via unsolResponseNewSms
     *
     * @param serial Serial number of request.
     * @param success is true on successful receipt
     *        (basically, AT+CNMA=1 from TS 27.005 is 0 on failed receipt
     *        (basically, AT+CNMA=2 from TS 27.005)
     * @param cause: if success is false, this contains the failure cause as defined
     *        in TS 23.040, 9.2.3.22.
     *
     * Response function is IRadioResponse.acknowledgeLastIncomingGsmSmsResponse()
     */
    void acknowledgeLastIncomingGsmSms(
            in int serial, in boolean success, in SmsAcknowledgeFailCause cause);

    /**
     * Reserves an unallocated pdu session id from the pool of ids. The allocated id is returned
     * in the response. When the id is no longer needed, call releasePduSessionId to return it to
     * the pool.
     *
     * Reference: 3GPP TS 24.007 section 11.2.3.1b
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.allocatePduSessionIdResponse()
     */
    void allocatePduSessionId(in int serial);

    /**
     * Whether uiccApplications are enabled, or disabled.
     * By default uiccApplications must be enabled, unless enableUiccApplications() with enable
     * being false is called.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.areUiccApplicationsEnabledResponse()
     */
    void areUiccApplicationsEnabled(in int serial);

    /**
     * Indicates that a handover was cancelled after a call to IRadio::startHandover.
     * Since the handover was unsuccessful, the modem retains ownership over any of the resources
     * being transferred and is still responsible for releasing them.
     *
     * @param serial Serial number of request.
     * @param id callId The identifier of the data call which is provided in SetupDataCallResult
     *
     * Response function is IRadioResponse.cancelHandoverResponse()
     */
    void cancelHandover(in int serial, in int callId);

    /**
     * Cancel the current USSD session if one exists.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.cancelPendingUssdResponse()
     */
    void cancelPendingUssd(in int serial);

    /**
     * Supplies old ICC PIN2 and new PIN2.
     *
     * @param serial Serial number of request.
     * @param oldPin2 Old pin2 value
     * @param newPin2 New pin2 value
     * @param aid AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     *
     * Response function is IRadioResponse.changeIccPin2ForAppResponse()
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
     * Response function is IRadioResponse.changeIccPinForAppResponse()
     */
    void changeIccPinForApp(in int serial, in String oldPin, in String newPin, in String aid);

    /**
     * Conference holding and active (like AT+CHLD=3)
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.conferenceResponse()
     */
    void conference(in int serial);

    /**
     * Deactivate packet data connection and remove from the data call list. An
     * unsolDataCallListChanged() must be sent when data connection is deactivated.
     *
     * @param serial Serial number of request.
     * @param cid Data call id.
     * @param reason The request reason. Must be normal, handover, or shutdown.
     *
     * Response function is IRadioResponse.deactivateDataCallResponse()
     */
    void deactivateDataCall(in int serial, in int cid, in DataRequestReason reason);

    /**
     * Deletes a CDMA SMS message from RUIM memory.
     *
     * @param serial Serial number of request.
     * @param index record index of the message to delete
     *
     * Response callback is IRadioResponse.deleteSmsOnRuimResponse()
     */
    void deleteSmsOnRuim(in int serial, in int index);

    /**
     * Deletes a SMS message from SIM memory.
     *
     * @param serial Serial number of request.
     * @param index Record index of the message to delete.
     *
     * Response function is IRadioResponse.deleteSmsOnSimResponse()
     */
    void deleteSmsOnSim(in int serial, in int index);

    /**
     * Initiate voice call. This method is never used for supplementary service codes.
     *
     * @param serial Serial number of request.
     * @param dialInfo Dial struct
     *
     * Response function is IRadioResponse.dialResponse()
     */
    void dial(in int serial, in Dial dialInfo);

    /**
     * Initiate emergency voice call, with zero or more emergency service category(s), zero or
     * more emergency Uniform Resource Names (URN), and routing information for handling the call.
     * Android uses this request to make its emergency call instead of using IRadio.dial if the
     * 'address' in the 'dialInfo' field is identified as an emergency number by Android.
     *
     * In multi-sim scenario, if the emergency number is from a specific subscription, this radio
     * request can still be sent out on the other subscription as long as routing is set to
     * EmergencyNumberRouting#EMERGENCY. This radio request will not be sent on an inactive
     * (PIN/PUK locked) subscription unless both subscriptions are PIN/PUK locked. In this case,
     * the request will be sent on the primary subscription.
     *
     * Some countries or carriers require some emergency numbers that must be handled with normal
     * call routing if possible or emergency routing. 1) if the 'routing' field is specified as
     * EmergencyNumberRouting#NORMAL, the implementation must try the full radio service to use
     * normal call routing to handle the call; if service cannot support normal routing, the
     * implementation must use emergency routing to handle the call. 2) if 'routing' is specified
     * as EmergencyNumberRouting#EMERGENCY, the implementation must use emergency routing to handle
     * the call. 3) if 'routing' is specified as EmergencyNumberRouting#UNKNOWN, Android does not
     * know how to handle the call.
     *
     * If the dialed emergency number does not have a specified emergency service category, the
     * 'categories' field is set to EmergencyServiceCategory#UNSPECIFIED; if the dialed emergency
     * number does not have specified emergency Uniform Resource Names, the 'urns' field is set to
     * an empty list. If the underlying technology used to request emergency services does not
     * support the emergency service category or emergency uniform resource names, the field
     * 'categories' or 'urns' may be ignored.
     *
     * In the scenarios that the 'address' in the 'dialInfo' field has other functions besides the
     * emergency number function, if the 'hasKnownUserIntentEmergency' field is true, the user's
     * intent for this dial request is emergency call, and the modem must treat this as an actual
     * emergency dial; if the 'hasKnownUserIntentEmergency' field is false, Android does not know
     * user's intent for this call.
     *
     * If 'isTesting' is true, this request is for testing purpose, and must not be sent to a real
     * emergency service; otherwise it's for a real emergency call request.
     *
     * Reference: 3gpp 22.101, Section 10 - Emergency Calls;
     *            3gpp 23.167, Section 6 - Functional description;
     *            3gpp 24.503, Section 5.1.6.8.1 - General;
     *            RFC 5031
     *
     * @param serial Serial number of request.
     * @param dialInfo the same Dial information used by IRadio.dial.
     * @param categories bitfield<EmergencyServiceCategory> the Emergency Service Category(s)
     *        of the call.
     * @param urns the emergency Uniform Resource Names (URN)
     * @param routing EmergencyCallRouting the emergency call routing information.
     * @param hasKnownUserIntentEmergency Flag indicating if user's intent for the emergency call
     *        is known.
     * @param isTesting Flag indicating if this request is for testing purpose.
     *
     * Response function is IRadioResponse.emergencyDialResponse()
     */
    void emergencyDial(in int serial, in Dial dialInfo, in EmergencyServiceCategory categories,
            in String[] urns, in EmergencyCallRouting routing,
            in boolean hasKnownUserIntentEmergency, in boolean isTesting);

    /**
     * Toggle logical modem on/off. This is similar to IRadio.setRadioPower(), however that
     * does not enforce that radio power is toggled only for the corresponding radio and certain
     * vendor implementations do it for all radios. This new API should affect only the modem for
     * which it is called. A modem stack must be on/active only when both setRadioPower() and
     * enableModem() are set to on for it.
     *
     * SIM must be read if available even if modem is off/inactive.
     *
     * @param serial Serial number of request.
     * @param on True to turn on the logical modem, otherwise turn it off.
     *
     * Response function is IRadioResponse.enableModemResponse()
     */
    void enableModem(in int serial, in boolean on);

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
     * Response callback is IRadioResponse.enableUiccApplicationsResponse()
     */
    void enableUiccApplications(in int serial, in boolean enable);

    /**
     * Request the radio's system selection module to exit emergency callback mode. Radio must not
     * respond with SUCCESS until the modem has completely exited from Emergency Callback Mode.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.exitEmergencyCallbackModeResponse()
     */
    void exitEmergencyCallbackMode(in int serial);

    /**
     * Connects the two calls and disconnects the subscriber from both calls.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.explicitCallTransferResponse()
     */
    void explicitCallTransfer(in int serial);

    /**
     * Get carrier restrictions.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getAllowedCarriersResponse()
     */
    void getAllowedCarriers(in int serial);

    /**
     * Requests bitmap representing the currently allowed network types.
     * getPreferredNetworkType, getPreferredNetworkTypesBitmap will not be called anymore
     * except for IRadio v1.5 or older devices.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getAllowedNetworkTypesBitmapResponse()
     */
    void getAllowedNetworkTypesBitmap(in int serial);

    /**
     * Get the list of band modes supported by RF.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getAvailableBandModesResponse()
     */
    void getAvailableBandModes(in int serial);

    /**
     * Scans for available networks
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getAvailableNetworksResponse()
     */
    void getAvailableNetworks(in int serial);

    /**
     * Get all the barring info for the current camped cell applicable to the current user.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getBarringInfoResponse()
     */
    void getBarringInfo(in int serial);

    /**
     * Return string value indicating baseband version, eg response from AT+CGMR
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getBasebandVersionResponse()
     */
    void getBasebandVersion(in int serial);

    /**
     * Request the device MDN / H_SID / H_NID. The request is only allowed when CDMA subscription
     * is available. When CDMA subscription is changed, application layer must re-issue the request
     * to update the subscription information.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getCDMASubscriptionResponse()
     */
    void getCDMASubscription(in int serial);

    /**
     * Request call forward status.
     *
     * @param serial Serial number of request.
     * @param callInfo CallForwardInfo
     *
     * Response function is IRadioResponse.getCallForwardStatusResponse()
     */
    void getCallForwardStatus(in int serial, in CallForwardInfo callInfo);

    /**
     * Query current call waiting state
     *
     * @param serial Serial number of request.
     * @param serviceClass Service class is the TS 27.007 service class to query
     *
     * Response function is IRadioResponse.getCallWaitingResponse()
     */
    void getCallWaiting(in int serial, in int serviceClass);

    /**
     * Request the setting of CDMA Broadcast SMS config
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getCdmaBroadcastConfigResponse()
     */
    void getCdmaBroadcastConfig(in int serial);

    /**
     * Request the actual setting of the roaming preferences in CDMA in the modem
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getCdmaRoamingPreferenceResponse()
     */
    void getCdmaRoamingPreference(in int serial);

    /**
     * Request to query the location where the CDMA subscription shall be retrieved.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getCdmaSubscriptionSourceResponse()
     */
    void getCdmaSubscriptionSource(in int serial);

    /**
     * Request all of the current cell information known to the radio. The radio must return a list
     * of all current cells, including the neighboring cells. If for a particular cell information
     * isn't known then the appropriate unknown value will be returned.
     * This does not cause or change the rate of unsolicited cellInfoList().
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getCellInfoListResponse()
     */
    void getCellInfoList(in int serial);

    /**
     * Queries the status of the CLIP supplementary service (for MMI code "*#30#")
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getClipResponse()
     */
    void getClip(in int serial);

    /**
     * Gets current CLIR status
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getClirResponse()
     */
    void getClir(in int serial);

    /**
     * Requests current call list
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getCurrentCallsResponse()
     */
    void getCurrentCalls(in int serial);

    /**
     * Returns the data call list. An entry is added when a setupDataCall() is issued and removed
     * on a deactivateDataCall(). The list is emptied when setRadioPower()  off/on issued or when
     * the vendor HAL or modem crashes.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getDataCallListResponse()
     */
    void getDataCallList(in int serial);

    /**
     * Request current data registration state.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getDataRegistrationStateResponse()
     */
    void getDataRegistrationState(in int serial);

    /**
     * Request the device ESN / MEID / IMEI / IMEISV. The request is always allowed and contains
     * GSM and CDMA device identity. When CDMA subscription is changed the ESN/MEID changes.
     * The application layer must re-issue the request to update the device identity in this case.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getDeviceIdentityResponse()
     */
    void getDeviceIdentity(in int serial);

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
     * Response function is IRadioResponse.getFacilityLockForAppResponse()
     */
    void getFacilityLockForApp(in int serial, in String facility, in String password,
            in int serviceClass, in String appId);

    /**
     * Request the setting of GSM/WCDMA Cell Broadcast SMS config.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getGsmBroadcastConfigResponse()
     */
    void getGsmBroadcastConfig(in int serial);

    /**
     * Request all of the current hardware (modem and sim) associated with Radio.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getHardwareConfigResponse()
     */
    void getHardwareConfig(in int serial);

    /**
     * Requests status of the ICC card
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getIccCardStatusResponse()
     *
     */
    void getIccCardStatus(in int serial);

    /**
     * Request current IMS registration state
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getImsRegistrationStateResponse()
     */
    void getImsRegistrationState(in int serial);

    /**
     * Get the SIM IMSI. Only valid when radio state is "RADIO_STATE_ON"
     *
     * @param serial Serial number of request.
     * @param aid AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     *
     * Response function is IRadioResponse.getImsiForAppResponse()
     *
     */
    void getImsiForApp(in int serial, in String aid);

    /**
     * Requests the failure cause code for the most recently terminated call.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getLastCallFailCauseResponse()
     *
     */
    void getLastCallFailCause(in int serial);

    /**
     * Get modem activity information for power consumption estimation. Request clear-on-read
     * statistics information that is used for estimating the per-millisecond power consumption
     * of the cellular modem.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getModemActivityInfoResponse()
     */
    void getModemActivityInfo(in int serial);

    /**
     * Request status of logical modem. It returns isEnabled=true if the logical modem is on.
     * This method is the getter method for enableModem.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getModemStackStatusResponse()
     */
    void getModemStackStatus(in int serial);

    /**
     * Queries the current state of the uplink mute setting
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getMuteResponse()
     */
    void getMute(in int serial);

    /**
     * Request neighboring cell id in GSM network
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getNeighboringCidsResponse()
     */
    void getNeighboringCids(in int serial);

    /**
     * Query current network selection mode
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getNetworkSelectionModeResponse()
     */
    void getNetworkSelectionMode(in int serial);

    /**
     * Request current operator ONS or EONS
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getOperatorResponse()
     */
    void getOperator(in int serial);

    /**
     * Query the preferred network type (CS/PS domain, RAT, and operation mode)
     * for searching and registering
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getPreferredNetworkTypeResponse()
     */
    void getPreferredNetworkType(in int serial);

    /**
     * Query the preferred network type bitmap.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getPreferredNetworkTypeBitmapResponse()
     */
    void getPreferredNetworkTypeBitmap(in int serial);

    /**
     * Request the setting of preferred voice privacy mode.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getPreferredVoicePrivacyResponse()
     */
    void getPreferredVoicePrivacy(in int serial);

    /**
     * Used to get phone radio capability.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getRadioCapabilityResponse()
     */
    void getRadioCapability(in int serial);

    /**
     * Requests current signal strength and associated information. Must succeed if radio is on.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getSignalStrengthResponse()
     */
    void getSignalStrength(in int serial);

    /**
     * Get the phone book capacity
     *
     * @param serial Serial number of request.
     *
     * Response function is defined from IRadioResponse.getSimPhonebookCapacityResponse()
     */
    void getSimPhonebookCapacity(in int serial);

    /**
     * Get the local and global phonebook records from the SIM card.
     * This should be called again after a simPhonebookChanged notification is received.
     * The phonebook records are received via IRadioIndication.simPhonebookRecordsReceived()
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getSimPhonebookRecordsResponse()
     */
    void getSimPhonebookRecords(in int serial);

    /**
     * Request to get the current slicing configuration including URSP rules and NSSAIs
     * (configured, allowed and rejected). URSP stands for UE route selection policy and is defined
     * in 3GPP TS 24.526 Section 4.2. An NSSAI is a collection of network slices. Each network slice
     * is identified by an S-NSSAI and is represented by the struct SliceInfo. NSSAI and S-NSSAI
     * are defined in 3GPP TS 24.501.
     *
     * Response function is IRadioResponse.getSlicingConfigResponse()
     */
    void getSlicingConfig(in int serial);

    /**
     * Get the default Short Message Service Center address on the device.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getSmscAddressResponse()
     */
    void getSmscAddress(in int serial);

    /**
     * Get which bands the modem's background scan is acting on.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getSystemSelectionChannelsResponse()
     */
    void getSystemSelectionChannels(in int serial);

    /**
     * Request the setting of TTY mode
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getTTYModeResponse()
     */
    void getTTYMode(in int serial);

    /**
     * Query the radio technology type (3GPP/3GPP2) used for voice. Query is valid only
     * when radio state is not RADIO_STATE_UNAVAILABLE
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.getVoiceRadioTechnologyResponse()
     */
    void getVoiceRadioTechnology(in int serial);

    /**
     * Request current voice registration state.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.getVoiceRegistrationStateResponse()
     */
    void getVoiceRegistrationState(in int serial);

    /**
     * When STK application gets stkCallSetup(), the call actually has been initialized by the
     * mobile device already. (We could see the call has been in the 'call list'). STK application
     * needs to accept/reject the call according to user operations.
     *
     * @param serial Serial number of request.
     * @param accept true = accept the call setup, false = reject the call setup
     *
     * Response callback is IRadioResponse.handleStkCallSetupRequestFromSimResponse()
     */
    void handleStkCallSetupRequestFromSim(in int serial, in boolean accept);

    /**
     * Hang up a specific line (like AT+CHLD=1x). After this HANGUP request returns, Radio must
     * show the connection is NOT active anymore in next getCurrentCalls() query.
     *
     * @param serial Serial number of request.
     * @param gsmIndex Connection index (value of 'x' in CHLD above)
     *
     * Response function is IRadioResponse.hangupResponse()
     */
    void hangup(in int serial, in int gsmIndex);

    /**
     * Hang up waiting or held (like AT+CHLD=1). After this HANGUP request returns, Radio must show
     * the connection is NOT active anymore in next getCurrentCalls() query.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.hangupForegroundResumeBackgroundResponse()
     */
    void hangupForegroundResumeBackground(in int serial);

    /**
     * Hang up waiting or held (like AT+CHLD=0). After this HANGUP request returns, Radio must show
     * the connection is NOT active anymore in next getCurrentCalls() query.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.hangupWaitingOrBackgroundResponse()
     */
    void hangupWaitingOrBackground(in int serial);

    /**
     * Close a previously opened logical channel. This command reflects TS 27.007
     * "close logical channel" operation (+CCHC).
     *
     * @param serial Serial number of request.
     * @param channelId session id of the logical channel (+CCHC).
     *
     * Response callback is IRadioResponse.iccCloseLogicalChannelResponse()
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
     * Response function is IRadioResponse.iccIOForAppResponse()
     */
    void iccIOForApp(in int serial, in IccIo iccIo);

    /**
     * Open a new logical channel and select the given application. This command
     * reflects TS 27.007 "open logical channel" operation (+CCHO).
     *
     * @param serial Serial number of request.
     * @param aid AID value, See ETSI 102.221 and 101.220.
     * @param p2 P2 value, described in ISO 7816-4. Ignore if equal to RadioConst:P2_CONSTANT_NO_P2
     *
     * Response callback is IRadioResponse.iccOpenLogicalChannelResponse()
     */
    void iccOpenLogicalChannel(in int serial, in String aid, in int p2);

    /**
     * Request APDU exchange on the basic channel. This command reflects TS 27.007
     * "generic SIM access" operation (+CSIM). The modem must ensure proper function of GSM/CDMA,
     * and filter commands appropriately. It must filter channel management and SELECT by DF
     * name commands. "sessionid" field must be ignored.
     *
     * @param serial Serial number of request.
     * @param message SimApdu as defined in types.hal to be sent
     *
     * Response callback is IRadioResponse.iccTransmitApduBasicChannelResponse()
     */
    void iccTransmitApduBasicChannel(in int serial, in SimApdu message);

    /**
     * Exchange APDUs with a UICC over a previously opened logical channel. This command reflects
     * TS 27.007 "generic logical channel access" operation (+CGLA). The modem must filter channel
     * management and SELECT by DF name commands.
     *
     * @param serial Serial number of request.
     * @param message SimApdu as defined in types.hal to be sent
     *
     * Response callback is IRadioResponse.iccTransmitApduLogicalChannelResponse()
     */
    void iccTransmitApduLogicalChannel(in int serial, in SimApdu message);

    /**
     * Is E-UTRA-NR Dual Connectivity enabled
     *
     * @param serial Serial number of request.
     * Response callback is IRadioResponse.isNrDualConnectivityEnabledResponse()
     */
    void isNrDualConnectivityEnabled(in int serial);

    /**
     * Read one of the radio NV items.
     * This is used for device configuration by some CDMA operators.
     *
     * @param serial Serial number of request.
     * @param itemId NvItem is radio NV item as defined in types.hal
     *
     * Response callback is IRadioResponse.nvReadItemResponse()
     */
    void nvReadItem(in int serial, in NvItem itemId);

    /**
     * Reset the radio NV configuration to the factory state.
     * This is used for device configuration by some CDMA operators.
     *
     * @param serial Serial number of request.
     * @param resetType ResetNvType as defined in types.hal
     *
     * Response callback is IRadioResponse.nvResetConfigResponse()
     */
    void nvResetConfig(in int serial, in ResetNvType resetType);

    /**
     * Update the CDMA Preferred Roaming List (PRL) in the radio NV storage.
     * This is used for device configuration by some CDMA operators.
     *
     * @param serial Serial number of request.
     * @param prl PRL as a byte array
     *
     * Response callback is IRadioResponse.nvWriteCdmaPrlResponse()
     */
    void nvWriteCdmaPrl(in int serial, in byte[] prl);

    /**
     * Write one of the radio NV items.
     * This is used for device configuration by some CDMA operators.
     *
     * @param serial Serial number of request.
     * @param item NvWriteItem as defined in types.hal
     *
     * Response callback is IRadioResponse.nvWriteItemResponse()
     */
    void nvWriteItem(in int serial, in NvWriteItem item);

    /**
     * Send UDUB (user determined user busy) to ringing or waiting call answer)
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.rejectCallResponse()
     */
    void rejectCall(in int serial);

    /**
     * Releases a pdu session id that was previously allocated using allocatePduSessionId.
     * Reference: 3GPP TS 24.007 section 11.2.3.1b
     *
     * @param serial Serial number of request.
     * @param id Pdu session id to release.
     *
     * Response function is IRadioResponse.releasePduSessionIdResponse()
     */
    void releasePduSessionId(in int serial, in int id);

    /**
     * Indicates whether there is storage available for new SMS messages.
     *
     * @param serial Serial number of request.
     * @param available true if memory is available for storing new messages,
     *        false if memory capacity is exceeded
     *
     * Response callback is IRadioResponse.reportSmsMemoryStatusResponse()
     */
    void reportSmsMemoryStatus(in int serial, in boolean available);

    /**
     * Indicates that the StkService is running and is ready to receive unsolicited stk commands.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.reportStkServiceIsRunningResponse()
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
     * Response callback is IRadioResponse.requestIccSimAuthenticationResponse()
     */
    void requestIccSimAuthentication(
            in int serial, in int authContext, in String authData, in String aid);

    /**
     * Request the ISIM application on the UICC to perform AKA challenge/response algorithm
     * for IMS authentication
     *
     * @param serial Serial number of request.
     * @param challenge challenge string in Base64 format
     *
     * Response callback is IRadioResponse.requestIsimAuthenticationResponse()
     */
    void requestIsimAuthentication(in int serial, in String challenge);

    /**
     * Device is shutting down. All further commands are ignored and RADIO_NOT_AVAILABLE
     * must be returned.
     *
     * @param serial Serial number of request.
     *
     * Response callback is IRadioResponse.requestShutdownResponse()
     */
    void requestShutdown(in int serial);

    /**
     * When response type received from a radio indication or radio response is
     * RadioIndicationType:UNSOLICITED_ACK_EXP or RadioResponseType:SOLICITED_ACK_EXP respectively,
     * acknowledge the receipt of those messages by sending responseAcknowledgement().
     */
    void responseAcknowledgement();

    /**
     * Send DTMF string
     *
     * @param serial Serial number of request.
     * @param dtmf DTMF string
     * @param on DTMF ON length in milliseconds, or 0 to use default
     * @param off is the DTMF OFF length in milliseconds, or 0 to use default
     *
     * Response callback is IRadioResponse.sendBurstDtmfResponse()
     */
    void sendBurstDtmf(in int serial, in String dtmf, in int on, in int off);

    /**
     * Send FLASH command
     *
     * @param serial Serial number of request.
     * @param featureCode String associated with Flash command
     *
     * Response callback is IRadioResponse.sendCDMAFeatureCodeResponse()
     */
    void sendCDMAFeatureCode(in int serial, in String featureCode);

    /**
     * Send a CDMA SMS message
     *
     * @param serial Serial number of request.
     * @param sms Cdma Sms to be sent described by CdmaSmsMessage in types.hal
     *
     * Response callback is IRadioResponse.sendCdmaSmsResponse()
     */
    void sendCdmaSms(in int serial, in CdmaSmsMessage sms);

    /**
     * Send an SMS message. Identical to sendCdmaSms, except that more messages are expected to be
     * sent soon.
     *
     * @param serial Serial number of request.
     * @param sms Cdma Sms to be sent described by CdmaSmsMessage in types.hal
     *
     * Response callback is IRadioResponse.sendCdmaSMSExpectMoreResponse()
     */
    void sendCdmaSmsExpectMore(in int serial, in CdmaSmsMessage sms);

    /**
     * Send the updated device state. This is providing the device state information for the modem
     * to perform power saving strategies.
     *
     * @param serial Serial number of request.
     * @param deviceStateType The updated device state type.
     * @param state The updated state. See the definition of state at DeviceStateType.
     *
     * Response callback is IRadioResponse.sendDeviceStateResponse()
     */
    void sendDeviceState(in int serial, in DeviceStateType deviceStateType, in boolean state);

    /**
     * Send a DTMF tone. If the implementation is currently playing a tone requested via
     * startDtmf(), that tone must be cancelled and the new tone must be played instead.
     *
     * @param serial Serial number of request.
     * @param s string with single char having one of 12 values: 0-9, *, #
     *
     * Response function is IRadioResponse.sendDtmfResponse()
     */
    void sendDtmf(in int serial, in String s);

    /**
     * Requests to send a SAT/USAT envelope command to SIM.
     * The SAT/USAT envelope command refers to 3GPP TS 11.14 and 3GPP TS 31.111
     *
     * @param serial Serial number of request.
     * @param command SAT/USAT command in hexadecimal format string starting with command tag
     *
     * Response function is IRadioResponse.sendEnvelopeResponse()
     */
    void sendEnvelope(in int serial, in String command);

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
     * Response callback is IRadioResponse.sendEnvelopeWithStatusResponse()
     */
    void sendEnvelopeWithStatus(in int serial, in String contents);

    /**
     * Send a SMS message over IMS. Based on the return error, caller decides to resend if sending
     * sms fails. SMS_SEND_FAIL_RETRY means retry, and other errors means no retry.
     * In case of retry, data is encoded based on Voice Technology available.
     *
     * @param serial Serial number of request.
     * @param message ImsSmsMessage as defined in types.hal to be sent
     *
     * Response callback is IRadioResponse.sendImsSmsResponse()
     */
    void sendImsSms(in int serial, in ImsSmsMessage message);

    /**
     * Send an SMS message. Based on the returned error, caller decides to resend if sending sms
     * fails. RadioError:SMS_SEND_FAIL_RETRY means retry (i.e. error cause is 332) and
     * RadioError:GENERIC_FAILURE means no retry (i.e. error cause is 500)
     *
     * @param serial Serial number of request.
     * @param message GsmSmsMessage as defined in types.hal
     *
     * Response function is IRadioResponse.sendSmsResponse()
     */
    void sendSms(in int serial, in GsmSmsMessage message);

    /**
     * Send an SMS message. Identical to sendSms, except that more messages are expected to be sent
     * soon. If possible, keep SMS relay protocol link open (eg TS 27.005 AT+CMMS command).
     * Based on the return error, caller decides to resend if sending sms fails.
     * RadioError:SMS_SEND_FAIL_RETRY means retry (i.e. error cause is 332) and
     * RadioError:GENERIC_FAILURE means no retry (i.e. error cause is 500)
     *
     * @param serial Serial number of request.
     * @param message GsmSmsMessage as defined in types.hal
     *
     * Response function is IRadioResponse.sendSmsExpectMoreResponse()
     */
    void sendSmsExpectMore(in int serial, in GsmSmsMessage message);

    /**
     * Requests to send a terminal response to SIM for a received proactive command
     *
     * @param serial Serial number of request.
     * @param commandResponse SAT/USAT response in hexadecimal format string starting with
     *        first byte of response data
     *
     * Response function is IRadioResponse.sendTerminalResponseResponseToSim()
     */
    void sendTerminalResponseToSim(in int serial, in String commandResponse);

    /**
     * Send a USSD message. If a USSD session already exists, the message must be sent in the
     * context of that session. Otherwise, a new session must be created. The network reply must be
     * reported via unsolOnUssd.
     *
     * Only one USSD session must exist at a time, and the session is assumed to exist until:
     * a) The android system invokes cancelUssd()
     * b) The implementation sends a unsolOnUssd() with a type code of
     *    "0" (USSD-Notify/no further action) or "2" (session terminated)
     *
     * @param serial Serial number of request.
     * @param ussd string containing the USSD request in UTF-8 format
     *
     * Response function is IRadioResponse.sendUssdResponse()
     *
     * See also requestCancelUssd, unsolOnUssd
     */
    void sendUssd(in int serial, in String ussd);

    /**
     * Separate a party from a multiparty call placing the multiparty call (less the specified
     * party) on hold and leaving the specified party as the only other member of the current
     * (active) call. Like AT+CHLD=2x.
     *
     * See TS 22.084 1.3.8.2 (iii)
     * TS 22.030 6.5.5 "Entering "2X followed by send"
     * TS 27.007 "AT+CHLD=2x"
     *
     * @param serial Serial number of request.
     * @param gsmIndex contains Connection index (value of 'x' in CHLD above)
     *
     * Response function is IRadioResponse.separateConnectionResponse()
     */
    void separateConnection(in int serial, in int gsmIndex);

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
     * Response callback is IRadioResponse.setAllowedCarriersResponse()
     */
    void setAllowedCarriers(in int serial, in CarrierRestrictions carriers,
            in SimLockMultiSimPolicy multiSimPolicy);

    /**
     * Requests to set the network type for searching and registering. Instruct the radio to
     * *only* accept the types of network provided. setPreferredNetworkTypesBitmap and
     * setPreferredNetworkType will not be called anymore except for IRadio v1.5 or older devices.
     * In case of an emergency call, the modem is authorized to bypass this restriction.
     *
     * @param serial Serial number of request.
     * @param networkTypeBitmap a 32-bit bearer bitmap of RadioAccessFamily
     *
     * Response callback is IRadioResponse.setAllowedNetworkTypesBitmapResponse()
     */
    void setAllowedNetworkTypesBitmap(in int serial, in RadioAccessFamily networkTypeBitmap);

    /**
     * Assign a specified band for RF configuration.
     *
     * @param serial Serial number of request.
     * @param mode RadioBandMode defined in types.hal
     *
     * Response function is IRadioResponse.setBandModeResponse()
     */
    void setBandMode(in int serial, in RadioBandMode mode);

    /**
     * Change call barring facility password
     *
     * @param serial Serial number of request.
     * @param facility facility string code from TS 27.007 7.4 (eg "AO" for BAOC)
     * @param oldPassword old password
     * @param newPassword new password
     *
     * Response function is IRadioResponse.setBarringPasswordResponse()
     */
    void setBarringPassword(
            in int serial, in String facility, in String oldPassword, in String newPassword);

    /**
     * Configure call forward rule
     *
     * @param serial Serial number of request.
     * @param callInfo CallForwardInfo
     *
     * Response function is IRadioResponse.setCallForwardResponse()
     */
    void setCallForward(in int serial, in CallForwardInfo callInfo);

    /**
     * Configure current call waiting state
     *
     * @param serial Serial number of request.
     * @param enable is false for "disabled" and true for "enabled"
     * @param serviceClass is the TS 27.007 service class bit vector of services to modify
     *
     * Response function is IRadioResponse.setCallWaitingResponse()
     */
    void setCallWaiting(in int serial, in boolean enable, in int serviceClass);

    /**
     * Provide Carrier specific information to the modem that must be used to encrypt the IMSI and
     * IMPI. Sent by the framework during boot, carrier switch and everytime the framework receives
     * a new certificate.
     *
     * @param serial Serial number of request.
     * @param imsiEncryptionInfo ImsiEncryptionInfo as defined in types.hal.
     *
     * Response callback is IRadioResponse.setCarrierInfoForImsiEncryptionResponse()
     */
    void setCarrierInfoForImsiEncryption(in int serial, in ImsiEncryptionInfo imsiEncryptionInfo);

    /**
     * Enable or disable the reception of CDMA Cell Broadcast SMS
     *
     * @param serial Serial number of request.
     * @param activate indicates to activate or turn off the reception of CDMA
     *        Cell Broadcast SMS. true = activate, false = turn off
     *
     * Response callback is IRadioResponse.setCdmaBroadcastActivationResponse()
     */
    void setCdmaBroadcastActivation(in int serial, in boolean activate);

    /**
     * Set CDMA Broadcast SMS config
     *
     * @param serial Serial number of request.
     * @param configInfo CDMA Broadcast SMS config to be set.
     *
     * Response callback is IRadioResponse.setCdmaBroadcastConfigResponse()
     */
    void setCdmaBroadcastConfig(in int serial, in CdmaBroadcastSmsConfigInfo[] configInfo);

    /**
     * Request to set the roaming preferences in CDMA
     *
     * @param serial Serial number of request.
     * @param type CdmaRoamingType defined in types.hal
     *
     * Response callback is IRadioResponse.setCdmaRoamingPreferenceResponse()
     */
    void setCdmaRoamingPreference(in int serial, in CdmaRoamingType type);

    /**
     * Request to set the location where the CDMA subscription shall be retrieved
     *
     * @param serial Serial number of request.
     * @param cdmaSub CdmaSubscriptionSource
     *
     * Response callback is IRadioResponse.setCdmaSubscriptionSourceResponse()
     */
    void setCdmaSubscriptionSource(in int serial, in CdmaSubscriptionSource cdmaSub);

    /**
     * Sets the minimum time between when unsolicited cellInfoList() must be invoked.
     * A value of 0, means invoke cellInfoList() when any of the reported information changes.
     * Setting the value to INT_MAX(0x7fffffff) means never issue a unsolicited cellInfoList().
     *
     * @param serial Serial number of request.
     * @param rate minimum time in milliseconds to indicate time between unsolicited cellInfoList()
     *
     * Response callback is IRadioResponse.setCellInfoListRateResponse()
     */
    void setCellInfoListRate(in int serial, in int rate);

    /**
     * Set current CLIR status
     *
     * @param serial Serial number of request.
     * @param status "n" parameter from TS 27.007 7.7
     *
     * Response function is IRadioResponse.setClirResponse()
     */
    void setClir(in int serial, in int status);

    /**
     * Tells the modem whether data calls are allowed or not
     *
     * @param serial Serial number of request.
     * @param allow true to allow data calls, false to disallow data calls
     *
     * Response callback is IRadioResponse.setDataAllowedResponse()
     */
    void setDataAllowed(in int serial, in boolean allow);

    /**
     * Send data profiles of the current carrier to the modem.
     *
     * @param serial Serial number of request.
     * @param profiles Array of DataProfileInfo to set.
     *
     * Response callback is IRadioResponse.setDataProfileResponse()
     */
    void setDataProfile(in int serial, in DataProfileInfo[] profiles);

    /**
     * Control data throttling at modem.
     * - DataThrottlingAction:NO_DATA_THROTTLING should clear any existing data throttling within
     *   the requested completion window.
     * - DataThrottlingAction:THROTTLE_SECONDARY_CARRIER: Remove any existing throttling on anchor
     *   carrier and achieve maximum data throttling on secondary carrier within the requested
     *   completion window.
     * - DataThrottlingAction:THROTTLE_ANCHOR_CARRIER: disable secondary carrier and achieve maximum
     *   data throttling on anchor carrier by requested completion window.
     * - DataThrottlingAction:HOLD: Immediately hold on to current level of throttling.
     *
     * @param serial Serial number of request.
     * @param dataThrottlingAction DataThrottlingAction as defined in types.hal
     * @param completionDurationMillis window, in milliseconds, in which the requested throttling
     *        action has to be achieved. This must be 0 when dataThrottlingAction is
     *        DataThrottlingAction:HOLD.
     *
     * Response function is IRadioResponse.setDataThrottlingResponse()
     */
    void setDataThrottling(in int serial, in DataThrottlingAction dataThrottlingAction,
            in long completionDurationMillis);

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
     * Response function is IRadioResponse.setFacilityLockForAppResponse()
     */
    void setFacilityLockForApp(in int serial, in String facility, in boolean lockState,
            in String password, in int serviceClass, in String appId);

    /**
     * Enable or disable the reception of GSM/WCDMA Cell Broadcast SMS
     *
     * @param serial Serial number of request.
     * @param activate indicates to activate or turn off the reception of GSM/WCDMA
     *        Cell Broadcast SMS. true = activate, false = turn off
     *
     * Response callback is IRadioResponse.setGsmBroadcastActivationResponse()
     */
    void setGsmBroadcastActivation(in int serial, in boolean activate);

    /**
     * Set GSM/WCDMA Cell Broadcast SMS config
     *
     * @param serial Serial number of request.
     * @param configInfo Setting of GSM/WCDMA Cell broadcast config
     *
     * Response callback is IRadioResponse.setGsmBroadcastConfigResponse()
     */
    void setGsmBroadcastConfig(in int serial, in GsmBroadcastSmsConfigInfo[] configInfo);

    /**
     * Sets the indication filter. Prevents the reporting of specified unsolicited indications from
     * the radio. This is used for power saving in instances when those indications are not needed.
     * If unset, defaults to IndicationFilter:ALL.
     *
     * @param serial Serial number of request.
     * @param indicationFilter 32-bit bitmap of IndicationFilter. Bits set to 1 indicate the
     *        indications are enabled. See IndicationFilter for the definition of each bit.
     *
     * Response callback is IRadioResponse.setIndicationFilterResponse()
     */
    void setIndicationFilter(in int serial, in IndicationFilter indicationFilter);

    /**
     * Set an APN to initial attach network.
     *
     * @param serial Serial number of request.
     * @param dataProfileInfo data profile containing APN settings
     *
     * Response callback is IRadioResponse.setInitialAttachApnResponse()
     */
    void setInitialAttachApn(in int serial, in DataProfileInfo dataProfileInfo);

    /**
     * Sets the link capacity reporting criteria. The resulting reporting criteria are the AND of
     * all the supplied criteria. Note that reporting criteria must be individually set for each
     * RAN. If unset, reporting criteria for that RAN are implementation-defined.
     *
     * Response callback is IRadioResponse.setLinkCapacityReportingCriteriaResponse().
     *
     * @param serial Serial number of request.
     * @param hysteresisMs A hysteresis time in milliseconds to prevent flapping. A value of 0
     *        disables hysteresis.
     * @param hysteresisDlKbps An interval in kbps defining the required magnitude change between DL
     *        reports. hysteresisDlKbps must be smaller than the smallest threshold delta. A value
     *        of 0 disables hysteresis.
     * @param hysteresisUlKbps An interval in kbps defining the required magnitude change between UL
     *        reports. hysteresisUlKbps must be smaller than the smallest threshold delta. A value
     *        of 0 disables hysteresis.
     * @param thresholdsDownlinkKbps A vector of trigger thresholds in kbps for downlink reports. A
     *        vector size of 0 disables the use of DL thresholds for reporting.
     * @param thresholdsUplinkKbps A vector of trigger thresholds in kbps for uplink reports. A
     *        vector size of 0 disables the use of UL thresholds for reporting.
     * @param accessNetwork The type of network for which to apply these thresholds.
     */
    void setLinkCapacityReportingCriteria(in int serial, in int hysteresisMs,
            in int hysteresisDlKbps, in int hysteresisUlKbps, in int[] thresholdsDownlinkKbps,
            in int[] thresholdsUplinkKbps, in AccessNetwork accessNetwork);

    /**
     * Enables/disables network state change notifications due to changes in LAC and/or CID (for
     * GSM) or BID/SID/NID/latitude/longitude (for CDMA). Basically +CREG=2 vs. +CREG=1 (TS 27.007).
     * The Radio implementation must default to "updates enabled" when the screen is on and
     * "updates disabled" when the screen is off.
     *
     * @param serial Serial number of request.
     * @param enable true=updates enabled (+CREG=2), false=updates disabled (+CREG=1)
     *
     * Response callback is IRadioResponse.setLocationUpdatesResponse()
     */
    void setLocationUpdates(in int serial, in boolean enable);

    /**
     * Turn on or off uplink (microphone) mute. Must only be sent while voice call is active.
     * Must always be reset to "disable mute" when a new voice call is initiated
     *
     * @param serial Serial number of request.
     * @param enable true for "enable mute" and false for "disable mute"
     *
     * Response function is IRadioResponse.setMuteResponse()
     */
    void setMute(in int serial, in boolean enable);

    /**
     * Specify that the network must be selected automatically.
     * This request must not respond until the new operator is selected and registered.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.setNetworkSelectionModeAutomaticResponse()
     */
    void setNetworkSelectionModeAutomatic(in int serial);

    /**
     * Manually select a specified network. This request must not respond until the new operator is
     * selected and registered. Per TS 23.122, the RAN is just the initial suggested value.
     * If registration fails, the RAN is not available afterwards, or the RAN is not within the
     * network types specified by IRadio::setPreferredNetworkTypeBitmap, then the modem will need to
     * select the next best RAN for network registration.
     *
     * @param serial Serial number of request.
     * @param operatorNumeric String specifying MCCMNC of network to select (eg "310170").
     * @param ran Initial suggested radio access network type. If value is UNKNOWN, the modem
     *        will select the next best RAN for network registration.
     *
     * Response function is IRadioResponse.setNetworkSelectionModeManualResponse()
     */
    void setNetworkSelectionModeManual(
            in int serial, in String operatorNumeric, in RadioAccessNetworks ran);

    /**
     * Enable or disable E-UTRA-NR dual connectivity. If disabled then UE will not connect
     * to secondary carrier.
     *
     * @param serial Serial number of request.
     * @param nrDualConnectivityState expected NR dual connectivity state.
     *        1: Enable NR dual connectivity {NrDualConnectivityState:ENABLE}
     *        2: Disable NR dual connectivity {NrDualConnectivityState:DISABLE}
     *        3: Disable NR dual connectivity and force secondary cell to be released
     *           {NrDualConnectivityState:DISABLE_IMMEDIATE}
     *
     * Response callback is IRadioResponse.setNRDualConnectivityStateResponse()
     */
    void setNrDualConnectivityState(
            in int serial, in NrDualConnectivityState nrDualConnectivityState);

    /**
     * Requests to set the preferred network type for searching and registering
     * (CS/PS domain, RAT, and operation mode)
     *
     * @param serial Serial number of request.
     * @param nwType PreferredNetworkType defined in types.hal
     *
     * Response callback is IRadioResponse.setPreferredNetworkTypeResponse()
     */
    void setPreferredNetworkType(in int serial, in PreferredNetworkType nwType);

    /**
     * Requests to set the preferred network type for searching and registering.
     *
     * @param serial Serial number of request.
     * @param networkTypeBitmap a 32-bit bitmap of RadioAccessFamily.
     *
     * Response callback is IRadioResponse.setPreferredNetworkTypeBitmapResponse()
     */
    void setPreferredNetworkTypeBitmap(in int serial, in RadioAccessFamily networkTypeBitmap);

    /**
     * Request to set the preferred voice privacy mode used in voice scrambling.
     *
     * @param serial Serial number of request.
     * @param enable false for Standard Privacy Mode (Public Long Code Mask)
     *        true for Enhanced Privacy Mode (Private Long Code Mask)
     *
     * Response callback is IRadioResponse.setPreferredVoicePrivacyResponse()
     */
    void setPreferredVoicePrivacy(in int serial, in boolean enable);

    /**
     * Used to set the phones radio capability. Be VERY careful using this request as it may cause
     * some vendor modems to reset. Because of the possible modem reset any radio commands after
     * this one may not be processed.
     *
     * @param serial Serial number of request.
     * @param rc RadioCapability structure to be set
     *
     * Response callback is IRadioResponse.setRadioCapabilityResponse()
     */
    void setRadioCapability(in int serial, in RadioCapability rc);

    /**
     * Toggle radio on and off (for "airplane" mode). If the radio is turned off/on the radio modem
     * subsystem is expected return to an initialized state. For instance, any voice and data calls
     * must be terminated and all associated lists emptied.
     * When setting radio power on to exit from airplane mode to place an emergency call on this
     * logical modem, powerOn, forEmergencyCall and preferredForEmergencyCall must be true. In
     * this case, this modem is optimized to scan only emergency call bands, until:
     * 1) Emergency call is completed; or
     * 2) Another setRadioPower is issued with forEmergencyCall being false or
     *    preferredForEmergencyCall being false; or
     * 3) Timeout after 30 seconds if dial or emergencyDial is not called.
     * Once one of these conditions is reached, the modem should move into normal operation.
     *
     * @param serial Serial number of request.
     * @param powerOn To turn on radio -> on = true, to turn off radio -> on = false.
     * @param forEmergencyCall To indication to radio if this request is due to emergency call.
     *        No effect if powerOn is false.
     * @param preferredForEmergencyCall indicate whether the following emergency call will be sent
     *        on this modem or not. No effect if forEmergencyCall is false, or powerOn is false.
     *
     * Response callback is IRadioConfigResponse.setRadioPowerResponse.
     */
    void setRadioPower(in int serial, in boolean powerOn, in boolean forEmergencyCall,
            in boolean preferredForEmergencyCall);

    /**
     * Set response functions for radio requests & radio indications.
     *
     * @param radioResponse Object containing response functions
     * @param radioIndication Object containing radio indications
     */
    void setResponseFunctions(in IRadioResponse radioResponse, in IRadioIndication radioIndication);

    /**
     * Sets the signal strength reporting criteria. The resulting reporting rules are the AND of all
     * the supplied criteria. For each RAN the hysteresisDb and thresholds apply to only the
     * following measured quantities:
     * -GERAN    - RSSI
     * -CDMA2000 - RSSI
     * -UTRAN    - RSCP
     * -EUTRAN   - RSRP/RSRQ/RSSNR
     * -NGRAN    - SSRSRP/SSRSRQ/SSSINR
     * Note that reporting criteria must be individually set for each RAN. For each RAN, if none of
     * reporting criteria of any measurement is set enabled (see SignalThresholdInfo.isEnabled),
     * the reporting criteria for this RAN is implementation-defined. For each RAN, if any reporting
     * criteria of any measure is set enabled, the reporting criteria of the other measures in this
     * RAN are set disabled (see SignalThresholdInfo.isEnabled) until they are set enabled.
     *
     * @param serial Serial number of request.
     * @param signalThresholdInfo Signal threshold info including the threshold values,
     *        hysteresisDb, hysteresisMs and isEnabled. See SignalThresholdInfo for details.
     * @param accessNetwork The type of network for which to apply these thresholds.
     *
     * Response callback is IRadioResponse.setSignalStrengthReportingCriteriaResponse()
     */
    void setSignalStrengthReportingCriteria(in int serial,
            in SignalThresholdInfo signalThresholdInfo, in AccessNetwork accessNetwork);

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
     * Response callback is IRadioResponse.setSimCardPowerResponse().
     */
    void setSimCardPower(in int serial, in CardPowerState powerUp);

    /**
     * Set the default Short Message Service Center address on the device.
     *
     * @param serial Serial number of request.
     * @param smsc Short Message Service Center address to set
     *
     * Response callback is IRadioResponse.setSmscAddressResponse()
     */
    void setSmscAddress(in int serial, in String smsc);

    /**
     * Enables/disables supplementary service related notifications from the network.
     * Notifications are reported via unsolSuppSvcNotification().
     *
     * @param serial Serial number of request.
     * @param enable true = notifications enabled, false = notifications disabled.
     *
     * Response function is IRadioResponse.setSuppServiceNotificationsResponse()
     */
    void setSuppServiceNotifications(in int serial, in boolean enable);

    /**
     * Specify which bands modem's background scan must act on. If specifyChannels is true, it only
     * scans bands specified in specifiers. If specifyChannels is false, it scans all bands. For
     * example, CBRS is only on LTE band 48. By specifying this band, modem saves more power.
     *
     * @param serial Serial number of request.
     * @param specifyChannels whether to scan bands defined in specifiers.
     * @param specifiers which bands to scan. Only used if specifyChannels is true.
     *
     * Response callback is IRadioResponse.setSystemSelectionChannelsResponse()
     */
    void setSystemSelectionChannels(
            in int serial, in boolean specifyChannels, in RadioAccessSpecifier[] specifiers);

    /**
     * Request to set the TTY mode
     *
     * @param serial Serial number of request.
     * @param mode TtyMode
     *
     * Response callback is IRadioResponse.setTTYModeResponse()
     */
    void setTTYMode(in int serial, in TtyMode mode);

    /**
     * Selection/de-selection of a subscription from a SIM card
     *
     * @param serial Serial number of request.
     * @param uiccSub SelectUiccSub as defined in types.hal
     *
     * Response callback is IRadioResponse.setUiccSubscriptionResponse()
     */
    void setUiccSubscription(in int serial, in SelectUiccSub uiccSub);

    /**
     * Setup a packet data connection. If DataCallResponse.status returns DataCallFailCause:NONE,
     * the data connection must be added to data calls and a unsolDataCallListChanged() must be
     * sent. The call remains until removed by subsequent unsolDataCallIstChanged(). It may be lost
     * due to many factors, including deactivateDataCall() being issued, the radio powered off,
     * reception lost or even transient factors like congestion. This data call list is returned by
     * getDataCallList() and dataCallListChanged().
     * The Radio is expected to:
     * - Create one data call context.
     * - Create and configure a dedicated interface for the context.
     * - The interface must be point to point.
     * - The interface is configured with one or more addresses and is capable of sending and
     *   receiving packets. The format is IP address with optional "/" prefix length (The format is
     *   defined in RFC-4291 section 2.3). For example, "192.0.1.3", "192.0.1.11/16", or
     *   "2001:db8::1/64". Typically one IPv4 or one IPv6 or one of each. If the prefix length is
     *   absent, then the addresses are assumed to be point to point with IPv4 with prefix length 32
     *   or IPv6 with prefix length 128.
     * - Must not modify routing configuration related to this interface; routing management is
     *   exclusively within the purview of the Android OS.
     * - Support simultaneous data call contexts up to DataRegStateResult.maxDataCalls specified in
     *   the response of getDataRegistrationState.
     *
     * @param serial Serial number of request.
     * @param accessNetwork The access network to setup the data call. If the data connection cannot
     *        be established on the specified access network then this should respond with an error.
     * @param dataProfileInfo Data profile info.
     * @param roamingAllowed Indicates whether or not data roaming is allowed by the user.
     * @param reason The request reason. Must be DataRequestReason:NORMAL or
     *        DataRequestReason:HANDOVER.
     * @param addresses If the reason is DataRequestReason:HANDOVER, this indicates the list of link
     *        addresses of the existing data connection. This parameter must be ignored unless
     *        reason is DataRequestReason:HANDOVER.
     * @param dnses If the reason is DataRequestReason:HANDOVER, this indicates the list of DNS
     *        addresses of the existing data connection. The format is defined in RFC-4291 section
     *        2.2. For example, "192.0.1.3" or "2001:db8::1". This parameter must be ignored unless
     *        reason is DataRequestReason:HANDOVER.
     * @param pduSessionId The pdu session id to be used for this data call. A value of 0 means no
     *        pdu session id was attached to this call. Reference: 3GPP TS 24.007 section 11.2.3.1b
     * @param sliceInfo SliceInfo to be used for the data connection when a handover occurs from
     *        EPDG to 5G. It is valid only when accessNetwork is AccessNetwork:NGRAN. If the slice
     *        passed from EPDG is rejected, then the data failure cause must be
     *        DataCallFailCause:SLICE_REJECTED.
     * @param trafficDescriptor TrafficDescriptor for which data connection needs to be established.
     *        It is used for URSP traffic matching as described in TS 24.526 Section 4.2.2.
     *        It includes an optional DNN which, if present, must be used for traffic matching --
     *        it does not specify the end point to be used for the data call. The end point is
     *        specified by DataProfileInfo.apn; DataProfileInfo.apn must be used as the end point if
     *        one is not specified through URSP rules.
     * @param matchAllRuleAllowed bool to indicate if using default match-all URSP rule for this
     *        request is allowed. If false, this request must not use the match-all URSP rule and if
     *        a non-match-all rule is not found (or if URSP rules are not available) it should
     *        return failure with cause DataCallFailCause:MATCH_ALL_RULE_NOT_ALLOWED. This is needed
     *        as some requests need to have a hard failure if the intention cannot be met, for
     *        example, a zero-rating slice.
     *
     * Response function is IRadioResponse.setupDataCallResponse()
     */
    void setupDataCall(in int serial, in AccessNetwork accessNetwork,
            in DataProfileInfo dataProfileInfo, in boolean roamingAllowed,
            in DataRequestReason reason, in LinkAddress[] addresses, in String[] dnses,
            in int pduSessionId, in @nullable SliceInfo sliceInfo,
            in @nullable TrafficDescriptor trafficDescriptor,
            in boolean matchAllRuleAllowed);

    /**
     * Start playing a DTMF tone. Continue playing DTMF tone until stopDtmf is received. If a
     * startDtmf() is received while a tone is currently playing, it must cancel the previous tone
     * and play the new one.
     *
     * @param serial Serial number of request.
     * @param s string having a single character with one of 12 values: 0-9,*,#
     *
     * Response function is IRadioResponse.startDtmfResponse()
     */
    void startDtmf(in int serial, in String s);

    /**
     * Indicates that a handover to the IWLAN transport has begun. Any resources being transferred
     * to the IWLAN transport cannot be released while a handover is underway. For example, if a
     * pdu session id needs to be transferred to IWLAN, then the modem should not release the id
     * while the handover is in progress. If a handover was unsuccessful, then the framework calls
     * IRadio::cancelHandover. The modem retains ownership over any of the resources being
     * transferred to IWLAN. If a handover was successful, the framework calls
     * IRadio::deactivateDataCall with reason HANDOVER. The IWLAN transport now owns the transferred
     * resources and is responsible for releasing them.
     *
     * @param serial Serial number of request.
     * @param id callId The identifier of the data call which is provided in SetupDataCallResult
     *
     * Response function is IRadioResponse.startHandoverResponse()
     */
    void startHandover(in int serial, in int callId);

    /**
     * Start a Keepalive session (for IPsec)
     *
     * @param serial Serial number of request.
     * @param keepalive A request structure containing all necessary info to describe a keepalive
     *
     * Response function is IRadioResponse.startKeepaliveResponse()
     */
    void startKeepalive(in int serial, in KeepaliveRequest keepalive);

    /**
     * Starts a network scan.
     *
     * @param serial Serial number of request.
     * @param request Defines the radio networks/bands/channels which need to be scanned.
     *
     * Response function is IRadioResponse.startNetworkScanResponse()
     */
    void startNetworkScan(in int serial, in NetworkScanRequest request);

    /**
     * Stop playing a currently playing DTMF tone.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.stopDtmfResponse()
     */
    void stopDtmf(in int serial);

    /**
     * Stop an ongoing Keepalive session (for IPsec)
     *
     * @param serial Serial number of request.
     * @param sessionHandle The handle that was provided by IRadioResponse.startKeepaliveResponse
     *
     * Response function is IRadioResponse.stopKeepaliveResponse()
     */
    void stopKeepalive(in int serial, in int sessionHandle);

    /**
     * Stops ongoing network scan
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.stopNetworkScanResponse()
     */
    void stopNetworkScan(in int serial);

    /**
     * Supplies ICC PIN2. Only called following operation where SIM_PIN2 was returned as a failure
     * from a previous operation.
     *
     * @param serial Serial number of request.
     * @param pin2 PIN2 value
     * @param aid AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     *
     * Response function is IRadioResponse.supplyIccPin2ForAppResponse()
     */
    void supplyIccPin2ForApp(in int serial, in String pin2, in String aid);

    /**
     * Supplies ICC PIN. Only called if CardStatus has AppState.PIN state
     *
     * @param serial Serial number of request.
     * @param pin PIN value
     * @param aid AID value, See ETSI 102.221 8.1 and 101.220 4, empty string if no value.
     *
     * Response function is IRadioResponse.supplyIccPinForAppResponse()
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
     * Response function is IRadioResponse.supplyIccPuk2ForAppResponse()
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
     * Response function is IRadioResponse.supplyIccPukForAppResponse()
     */
    void supplyIccPukForApp(in int serial, in String puk, in String pin, in String aid);

    /**
     * Requests that network personalization be deactivated
     *
     * @param serial Serial number of request.
     * @param netPin Network depersonlization code
     *
     * Response function is IRadioResponse.supplyNetworkDepersonalizationResponse()
     */
    void supplyNetworkDepersonalization(in int serial, in String netPin);

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
     * Response function is IRadioResponse.supplySimDepersonalizationResponse()
     */
    void supplySimDepersonalization(
            in int serial, in PersoSubstate persoType, in String controlKey);

    /**
     * Switch waiting or holding call and active call (like AT+CHLD=2).
     * Call transitions must happen as shown below.
     *   BEFORE                               AFTER
     * Call 1   Call 2                 Call 1       Call 2
     * ACTIVE   HOLDING                HOLDING     ACTIVE
     * ACTIVE   WAITING                HOLDING     ACTIVE
     * HOLDING  WAITING                HOLDING     ACTIVE
     * ACTIVE   IDLE                   HOLDING     IDLE
     * IDLE     IDLE                   IDLE        IDLE
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioResponse.switchWaitingOrHoldingAndActiveResponse()
     */
    void switchWaitingOrHoldingAndActive(in int serial);

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
     * Response callback is IRadioResponse.updateSimPhonebookRecordsResponse()
     */
    void updateSimPhonebookRecords(in int serial, in PhonebookRecordInfo recordInfo);

    /**
     * Stores a CDMA SMS message to RUIM memory.
     *
     * @param serial Serial number of request.
     * @param cdmaSms CDMA message as defined by CdmaSmsWriteArgs in types.hal
     *
     * Response callback is IRadioResponse.writeSmsToRuimResponse()
     */
    void writeSmsToRuim(in int serial, in CdmaSmsWriteArgs cdmaSms);

    /**
     * Stores a SMS message to SIM memory.
     *
     * @param serial Serial number of request.
     * @param smsWriteArgs SmsWriteArgs defined in types.hal
     *
     * Response function is IRadioResponse.writeSmsToSimResponse()
     */
    void writeSmsToSim(in int serial, in SmsWriteArgs smsWriteArgs);
}
