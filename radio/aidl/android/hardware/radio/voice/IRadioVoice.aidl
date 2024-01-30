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

package android.hardware.radio.voice;

import android.hardware.radio.voice.CallForwardInfo;
import android.hardware.radio.voice.Dial;
import android.hardware.radio.voice.EmergencyCallRouting;
import android.hardware.radio.voice.IRadioVoiceIndication;
import android.hardware.radio.voice.IRadioVoiceResponse;
import android.hardware.radio.voice.TtyMode;

/**
 * This interface is used by telephony and telecom to talk to cellular radio for voice APIs.
 * All the functions have minimum one parameter:
 * serial: which corresponds to serial no. of request. Serial numbers must only be memorized for the
 * duration of a method call. If clients provide colliding serials (including passing the same
 * serial to different methods), multiple responses (one for each method call) must still be served.
 * setResponseFunctions must work with IRadioVoiceResponse and IRadioVoiceIndication.
 * @hide
 */
@VintfStability
oneway interface IRadioVoice {
    /**
     * Answer incoming call. Must not be called for WAITING calls.
     * switchWaitingOrHoldingAndActive() must be used in this case instead
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.acceptCallResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void acceptCall(in int serial);

    /**
     * Cancel the current USSD session if one exists.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.cancelPendingUssdResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void cancelPendingUssd(in int serial);

    /**
     * Conference holding and active (like AT+CHLD=3)
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.conferenceResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void conference(in int serial);

    /**
     * Initiate voice call. This method is never used for supplementary service codes.
     *
     * @param serial Serial number of request.
     * @param dialInfo Dial struct
     *
     * Response function is IRadioVoiceResponse.dialResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
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
     * @param dialInfo the same Dial information used by IRadioVoice.dial.
     * @param categories bitfield<EmergencyServiceCategory> the Emergency Service Category(s)
     *        of the call.
     * @param urns the emergency Uniform Resource Names (URN)
     * @param routing EmergencyCallRouting the emergency call routing information.
     * @param hasKnownUserIntentEmergency Flag indicating if user's intent for the emergency call
     *        is known.
     * @param isTesting Flag indicating if this request is for testing purpose.
     *
     * Response function is IRadioVoiceResponse.emergencyDialResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void emergencyDial(in int serial, in Dial dialInfo, in int categories, in String[] urns,
            in EmergencyCallRouting routing, in boolean hasKnownUserIntentEmergency,
            in boolean isTesting);

    /**
     * Request the radio's system selection module to exit emergency callback mode. Radio must not
     * respond with SUCCESS until the modem has completely exited from Emergency Callback Mode.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.exitEmergencyCallbackModeResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void exitEmergencyCallbackMode(in int serial);

    /**
     * Connects the two calls and disconnects the subscriber from both calls.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.explicitCallTransferResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void explicitCallTransfer(in int serial);

    /**
     * Request call forward status.
     *
     * @param serial Serial number of request.
     * @param callInfo CallForwardInfo
     *
     * Response function is IRadioVoiceResponse.getCallForwardStatusResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void getCallForwardStatus(in int serial, in CallForwardInfo callInfo);

    /**
     * Query current call waiting state
     *
     * @param serial Serial number of request.
     * @param serviceClass Service class is the TS 27.007 service class to query
     *
     * Response function is IRadioVoiceResponse.getCallWaitingResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void getCallWaiting(in int serial, in int serviceClass);

    /**
     * Queries the status of the CLIP supplementary service (for MMI code "*#30#")
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.getClipResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void getClip(in int serial);

    /**
     * Gets current CLIR status
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.getClirResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void getClir(in int serial);

    /**
     * Requests current call list
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.getCurrentCallsResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void getCurrentCalls(in int serial);

    /**
     * Requests the failure cause code for the most recently terminated call.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.getLastCallFailCauseResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void getLastCallFailCause(in int serial);

    /**
     * Queries the current state of the uplink mute setting
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.getMuteResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void getMute(in int serial);

    /**
     * Request the setting of preferred voice privacy mode.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.getPreferredVoicePrivacyResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void getPreferredVoicePrivacy(in int serial);

    /**
     * Request the setting of TTY mode
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.getTtyModeResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void getTtyMode(in int serial);

    /**
     * When STK application gets stkCallSetup(), the call actually has been initialized by the
     * mobile device already. (We could see the call has been in the 'call list'). STK application
     * needs to accept/reject the call according to user operations.
     *
     * @param serial Serial number of request.
     * @param accept true = accept the call setup, false = reject the call setup
     *
     * Response function is IRadioVoiceResponse.handleStkCallSetupRequestFromSimResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void handleStkCallSetupRequestFromSim(in int serial, in boolean accept);

    /**
     * Hang up a specific line (like AT+CHLD=1x). After this HANGUP request returns, Radio must
     * show the connection is NOT active anymore in next getCurrentCalls() query.
     *
     * @param serial Serial number of request.
     * @param gsmIndex Connection index (value of 'x' in CHLD above)
     *
     * Response function is IRadioVoiceResponse.hangupResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void hangup(in int serial, in int gsmIndex);

    /**
     * Hang up waiting or held (like AT+CHLD=1). After this HANGUP request returns, Radio must show
     * the connection is NOT active anymore in next getCurrentCalls() query.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.hangupForegroundResumeBackgroundResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void hangupForegroundResumeBackground(in int serial);

    /**
     * Hang up waiting or held (like AT+CHLD=0). After this HANGUP request returns, Radio must show
     * the connection is NOT active anymore in next getCurrentCalls() query.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.hangupWaitingOrBackgroundResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void hangupWaitingOrBackground(in int serial);

    /**
     * Query current Voice NR enable state
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.isVoNrEnabledResponse()
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void isVoNrEnabled(in int serial);

    /**
     * Send UDUB (user determined user busy) to ringing or waiting call answer)
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.rejectCallResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void rejectCall(in int serial);

    /**
     * When response type received from a radio indication or radio response is
     * RadioIndicationType:UNSOLICITED_ACK_EXP or RadioResponseType:SOLICITED_ACK_EXP respectively,
     * acknowledge the receipt of those messages by sending responseAcknowledgement().
     *
     * This is available when android.hardware.telephony.calling is defined.
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
     * Response function is IRadioVoiceResponse.sendBurstDtmfResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void sendBurstDtmf(in int serial, in String dtmf, in int on, in int off);

    /**
     * Send FLASH command
     *
     * @param serial Serial number of request.
     * @param featureCode String associated with Flash command
     *
     * Response function is IRadioVoiceResponse.sendCdmaFeatureCodeResponse()
     *
     * This is available when android.hardware.telephony.cdma is defined.
     */
    void sendCdmaFeatureCode(in int serial, in String featureCode);

    /**
     * Send a DTMF tone. If the implementation is currently playing a tone requested via
     * startDtmf(), that tone must be cancelled and the new tone must be played instead.
     *
     * @param serial Serial number of request.
     * @param s string with single char having one of 12 values: 0-9, *, #
     *
     * Response function is IRadioVoiceResponse.sendDtmfResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void sendDtmf(in int serial, in String s);

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
     * Response function is IRadioVoiceResponse.sendUssdResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
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
     * Response function is IRadioVoiceResponse.separateConnectionResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void separateConnection(in int serial, in int gsmIndex);

    /**
     * Configure call forward rule
     *
     * @param serial Serial number of request.
     * @param callInfo CallForwardInfo
     *
     * Response function is IRadioVoiceResponse.setCallForwardResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void setCallForward(in int serial, in CallForwardInfo callInfo);

    /**
     * Configure current call waiting state
     *
     * @param serial Serial number of request.
     * @param enable is false for "disabled" and true for "enabled"
     * @param serviceClass is the TS 27.007 service class bit vector of services to modify
     *
     * Response function is IRadioVoiceResponse.setCallWaitingResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void setCallWaiting(in int serial, in boolean enable, in int serviceClass);

    /**
     * Set current CLIR status
     *
     * @param serial Serial number of request.
     * @param status "n" parameter from TS 27.007 7.7
     *
     * Response function is IRadioVoiceResponse.setClirResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void setClir(in int serial, in int status);

    /**
     * Turn on or off uplink (microphone) mute. Must only be sent while voice call is active.
     * Must always be reset to "disable mute" when a new voice call is initiated
     *
     * @param serial Serial number of request.
     * @param enable true for "enable mute" and false for "disable mute"
     *
     * Response function is IRadioVoiceResponse.setMuteResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void setMute(in int serial, in boolean enable);

    /**
     * Request to set the preferred voice privacy mode used in voice scrambling.
     *
     * @param serial Serial number of request.
     * @param enable false for Standard Privacy Mode (Public Long Code Mask)
     *        true for Enhanced Privacy Mode (Private Long Code Mask)
     *
     * Response function is IRadioVoiceResponse.setPreferredVoicePrivacyResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void setPreferredVoicePrivacy(in int serial, in boolean enable);

    /**
     * Set response functions for voice radio requests and indications.
     *
     * @param radioVoiceResponse Object containing response functions
     * @param radioVoiceIndication Object containing radio indications
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void setResponseFunctions(in IRadioVoiceResponse radioVoiceResponse,
            in IRadioVoiceIndication radioVoiceIndication);

    /**
     * Request to set the TTY mode
     *
     * @param serial Serial number of request.
     * @param mode TtyMode
     *
     * Response function is IRadioVoiceResponse.setTtyModeResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void setTtyMode(in int serial, in TtyMode mode);

    /**
     * Set Voice NR enable state
     *
     * @param serial Serial number of request.
     * @param enable true for "enable vonr" and false for "disable vonr"
     *
     * Response function is IRadioVoiceResponse.setVoNrEnabledResponse()
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void setVoNrEnabled(in int serial, in boolean enable);

    /**
     * Start playing a DTMF tone. Continue playing DTMF tone until stopDtmf is received. If a
     * startDtmf() is received while a tone is currently playing, it must cancel the previous tone
     * and play the new one.
     *
     * @param serial Serial number of request.
     * @param s string having a single character with one of 12 values: 0-9,*,#
     *
     * Response function is IRadioVoiceResponse.startDtmfResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void startDtmf(in int serial, in String s);

    /**
     * Stop playing a currently playing DTMF tone.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioVoiceResponse.stopDtmfResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void stopDtmf(in int serial);

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
     * Response function is IRadioVoiceResponse.switchWaitingOrHoldingAndActiveResponse()
     *
     * This is available when android.hardware.telephony.calling is defined.
     */
    void switchWaitingOrHoldingAndActive(in int serial);
}
