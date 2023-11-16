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

package android.hardware.radio.modem;

import android.hardware.radio.modem.DeviceStateType;
import android.hardware.radio.modem.IRadioModemIndication;
import android.hardware.radio.modem.IRadioModemResponse;
import android.hardware.radio.modem.NvItem;
import android.hardware.radio.modem.NvWriteItem;
import android.hardware.radio.modem.RadioCapability;
import android.hardware.radio.modem.ResetNvType;

/**
 * This interface is used by telephony and telecom to talk to cellular radio for modem APIs.
 * All the functions have minimum one parameter:
 * serial: which corresponds to serial no. of request. Serial numbers must only be memorized for the
 * duration of a method call. If clients provide colliding serials (including passing the same
 * serial to different methods), multiple responses (one for each method call) must still be served.
 * setResponseFunctions must work with IRadioModemResponse and IRadioModemIndication.
 * @hide
 */
@VintfStability
oneway interface IRadioModem {
    /**
     * Toggle logical modem on/off. This is similar to IRadioModem.setRadioPower(), however that
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
     * Response function is IRadioModemResponse.enableModemResponse()
     *
     * This is available when android.hardware.telephony is defined.
     */
    void enableModem(in int serial, in boolean on);

    /**
     * Return string value indicating baseband version, eg response from AT+CGMR
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioModemResponse.getBasebandVersionResponse()
     *
     * This is available when android.hardware.telephony is defined.
     */
    void getBasebandVersion(in int serial);

    /**
     * Request the device ESN / MEID / IMEI / IMEISV. The request is always allowed and contains
     * GSM and CDMA device identity. When CDMA subscription is changed the ESN/MEID changes.
     * The application layer must re-issue the request to update the device identity in this case.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioModemResponse.getDeviceIdentityResponse()
     *
     * This is available when android.hardware.telephony is defined.
     * @deprecated use getImei(int serial)
     */
    void getDeviceIdentity(in int serial);

    /**
     * Request all of the current hardware (modem and sim) associated with Radio.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioModemResponse.getHardwareConfigResponse()
     *
     * This is available when android.hardware.telephony is defined.
     */
    void getHardwareConfig(in int serial);

    /**
     * Get modem activity information for power consumption estimation. Request clear-on-read
     * statistics information that is used for estimating the per-millisecond power consumption
     * of the cellular modem.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioModemResponse.getModemActivityInfoResponse()
     *
     * This is available when android.hardware.telephony.radio.access is defined.
     */
    void getModemActivityInfo(in int serial);

    /**
     * Request status of logical modem. It returns isEnabled=true if the logical modem is on.
     * This method is the getter method for enableModem.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioModemResponse.getModemStackStatusResponse()
     *
     * This is available when android.hardware.telephony is defined.
     */
    void getModemStackStatus(in int serial);

    /**
     * Get phone radio capability.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioModemResponse.getRadioCapabilityResponse()
     *
     * This is available when android.hardware.telephony.radio.access is defined.
     */
    void getRadioCapability(in int serial);

    /**
     * Read one of the radio NV items.
     * This is used for device configuration by some CDMA operators.
     *
     * @param serial Serial number of request.
     * @param itemId NvItem
     *
     * Response function is IRadioModemResponse.nvReadItemResponse()
     *
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    void nvReadItem(in int serial, in NvItem itemId);

    /**
     * Reset the radio NV configuration to the factory state.
     * This is used for device configuration by some CDMA operators.
     *
     * @param serial Serial number of request.
     * @param resetType ResetNvType
     *
     * Response function is IRadioModemResponse.nvResetConfigResponse()
     *
     * Note: This will be deprecated in favor of a rebootModem API in Android U.
     */
    void nvResetConfig(in int serial, in ResetNvType resetType);

    /**
     * Update the CDMA Preferred Roaming List (PRL) in the radio NV storage.
     * This is used for device configuration by some CDMA operators.
     *
     * @param serial Serial number of request.
     * @param prl PRL as a byte array
     *
     * Response function is IRadioModemResponse.nvWriteCdmaPrlResponse()
     *
     * This is available when android.hardware.telephony.cdma is defined.
     *
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    void nvWriteCdmaPrl(in int serial, in byte[] prl);

    /**
     * Write one of the radio NV items.
     * This is used for device configuration by some CDMA operators.
     *
     * @param serial Serial number of request.
     * @param item NvWriteItem
     *
     * Response function is IRadioModemResponse.nvWriteItemResponse()
     *
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    void nvWriteItem(in int serial, in NvWriteItem item);

    /**
     * Device is shutting down. All further commands are ignored and RADIO_NOT_AVAILABLE
     * must be returned.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioModemResponse.requestShutdownResponse()
     *
     * This is available when android.hardware.telephony.radio.access is defined.
     */
    void requestShutdown(in int serial);

    /**
     * When response type received from a radio indication or radio response is
     * RadioIndicationType:UNSOLICITED_ACK_EXP or RadioResponseType:SOLICITED_ACK_EXP respectively,
     * acknowledge the receipt of those messages by sending responseAcknowledgement().
     *
     * This is available when android.hardware.telephony is defined.
     */
    void responseAcknowledgement();

    /**
     * Send the updated device state. This is providing the device state information for the modem
     * to perform power saving strategies.
     *
     * @param serial Serial number of request.
     * @param deviceStateType The updated device state type.
     * @param state The updated state. See the definition of state at DeviceStateType.
     *
     * Response function is IRadioModemResponse.sendDeviceStateResponse()
     *
     * This is available when android.hardware.telephony is defined.
     */
    void sendDeviceState(in int serial, in DeviceStateType deviceStateType, in boolean state);

    /**
     * Used to set the phones radio capability. Be VERY careful using this request as it may cause
     * some vendor modems to reset. Because of the possible modem reset any radio commands after
     * this one may not be processed.
     *
     * @param serial Serial number of request.
     * @param rc RadioCapability structure to be set
     *
     * Response function is IRadioModemResponse.setRadioCapabilityResponse()
     *
     * This is available when android.hardware.telephony.radio.access is defined.
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
     * Response function is IRadioConfigResponse.setRadioPowerResponse()
     *
     * This is available when android.hardware.telephony.radio.access is defined.
     */
    void setRadioPower(in int serial, in boolean powerOn, in boolean forEmergencyCall,
            in boolean preferredForEmergencyCall);

    /**
     * Set response functions for modem radio requests and indications.
     *
     * @param radioModemResponse Object containing response functions
     * @param radioModemIndication Object containing radio indications
     *
     * This is available when android.hardware.telephony is defined.
     */
    void setResponseFunctions(in IRadioModemResponse radioModemResponse,
            in IRadioModemIndication radioModemIndication);

    /**
     * Request the IMEI associated with the radio.
     *
     * @param serial : Serial number of request.
     *
     * Response function is IRadioModemResponse.getImeiResponse()
     *
     * This is available when android.hardware.telephony.gsm is defined.
     */
    void getImei(in int serial);
}
