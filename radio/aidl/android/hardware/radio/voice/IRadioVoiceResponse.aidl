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

import android.hardware.radio.RadioResponseInfo;
import android.hardware.radio.voice.Call;
import android.hardware.radio.voice.CallForwardInfo;
import android.hardware.radio.voice.ClipStatus;
import android.hardware.radio.voice.LastCallFailCauseInfo;
import android.hardware.radio.voice.TtyMode;

/**
 * Interface declaring response functions to solicited radio requests for voice APIs.
 * @hide
 */
@VintfStability
oneway interface IRadioVoiceResponse {
    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:SYSTEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void acceptCallResponse(in RadioResponseInfo info);

    /**
     * Acknowledge the receipt of radio request sent to the vendor. This must be sent only for
     * radio request which take long time to respond. For more details, refer
     * https://source.android.com/devices/tech/connect/ril.html
     *
     * @param serial Serial no. of the request whose acknowledgement is sent.
     */
    void acknowledgeRequest(in int serial);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SIM_BUSY
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void cancelPendingUssdResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_CALL_ID
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void conferenceResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:DIAL_MODIFIED_TO_USSD
     *   RadioError:DIAL_MODIFIED_TO_SS
     *   RadioError:DIAL_MODIFIED_TO_DIAL
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:INVALID_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:INTERNAL_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:MODEM_ERR
     *   RadioError:NO_SUBSCRIPTION
     *   RadioError:NO_NETWORK_FOUND
     *   RadioError:INVALID_CALL_ID
     *   RadioError:DEVICE_IN_USE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:ABORTED
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:CANCELLED
     */
    void dialResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:DIAL_MODIFIED_TO_USSD
     *   RadioError:DIAL_MODIFIED_TO_SS
     *   RadioError:DIAL_MODIFIED_TO_DIAL
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:INTERNAL_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:MODEM_ERR
     *   RadioError:NO_SUBSCRIPTION
     *   RadioError:NO_NETWORK_FOUND
     *   RadioError:INVALID_CALL_ID
     *   RadioError:DEVICE_IN_USE
     *   RadioError:ABORTED
     *   RadioError:INVALID_MODEM_STATE
     */
    void emergencyDialResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NO_ALLOWED
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void exitEmergencyCallbackModeResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void explicitCallTransferResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param callForwardInfos points to a vector of CallForwardInfo, one for each distinct
     *        registered phone number. For example, if data is forwarded to +18005551212 and voice
     *        is forwarded to +18005559999, then two separate CallForwardInfo's must be returned.
     *        However, if both data and voice are forwarded to +18005551212, then a single
     *        CallForwardInfo must be returned with the service class set to "data + voice = 3".
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SS_MODIFIED_TO_DIAL
     *   RadioError:SS_MODIFIED_TO_USSD
     *   RadioError:SS_MODIFIED_TO_SS
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:SYSTEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getCallForwardStatusResponse(
            in RadioResponseInfo info, in CallForwardInfo[] callForwardInfos);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param enable If current call waiting state is disabled, enable = false else true
     * @param serviceClass If enable, then callWaitingResp[1] must follow, with the TS 27.007
     *        service class bit vector of services for which call waiting is enabled. For example,
     *        if callWaitingResp[0] is 1 and callWaitingResp[1] is 3, then call waiting is enabled
     *        for data and voice and disabled for everything else.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SS_MODIFIED_TO_DIAL
     *   RadioError:SS_MODIFIED_TO_USSD
     *   RadioError:SS_MODIFIED_TO_SS
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getCallWaitingResponse(in RadioResponseInfo info, in boolean enable, in int serviceClass);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param status indicates CLIP status
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getClipResponse(in RadioResponseInfo info, in ClipStatus status);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param n is "n" parameter from TS 27.007 7.7
     * @param m is "m" parameter from TS 27.007 7.7
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SS_MODIFIED_TO_DIAL
     *   RadioError:SS_MODIFIED_TO_USSD
     *   RadioError:SS_MODIFIED_TO_SS
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getClirResponse(in RadioResponseInfo info, in int n, in int m);

    /**
     * @param info Response info struct containing respontype, serial no. and error
     * @param calls Current call list
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getCurrentCallsResponse(in RadioResponseInfo info, in Call[] calls);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param failCauseInfo Contains LastCallFailCause and vendor cause code.
     *
     * The vendor cause code must be used for debugging purpose only. The implementation must return
     * one of the values of LastCallFailCause as mentioned below.
     * GSM failure reasons codes for the cause codes defined in TS 24.008 Annex H where possible.
     * CDMA failure reasons codes for the possible call failure scenarios described in the
     * "CDMA IS-2000 Release A (C.S0005-A v6.0)" standard.
     * Any of the following reason codes if the call is failed or dropped due to reason mentioned
     * with in the braces.
     *   LastCallFailCause:RADIO_OFF (Radio is OFF)
     *   LastCallFailCause:OUT_OF_SERVICE (No cell coverage)
     *   LastCallFailCause:NO_VALID_SIM (No valid SIM)
     *   LastCallFailCause:RADIO_INTERNAL_ERROR (Modem hit unexpected error scenario)
     *   LastCallFailCause:NETWORK_RESP_TIMEOUT (No response from network)
     *   LastCallFailCause:NETWORK_REJECT (Explicit network reject)
     *   LastCallFailCause:RADIO_ACCESS_FAILURE (RRC connection failure. Eg.RACH)
     *   LastCallFailCause:RADIO_LINK_FAILURE (Radio Link Failure)
     *   LastCallFailCause:RADIO_LINK_LOST (Radio link lost due to poor coverage)
     *   LastCallFailCause:RADIO_UPLINK_FAILURE (Radio uplink failure)
     *   LastCallFailCause:RADIO_SETUP_FAILURE (RRC connection setup failure)
     *   LastCallFailCause:RADIO_RELEASE_NORMAL (RRC connection release, normal)
     *   LastCallFailCause:RADIO_RELEASE_ABNORMAL (RRC connection release, abnormal)
     *   LastCallFailCause:ACCESS_CLASS_BLOCKED (Access class barring)
     *   LastCallFailCause:NETWORK_DETACH (Explicit network detach)
     *   OEM causes (LastCallFailCause:OEM_CAUSE_XX) must be used for debug purpose only
     *
     * If the implementation does not have access to the exact cause codes, then it must return one
     * of the values listed in LastCallFailCause, as the UI layer needs to distinguish these cases
     * for tone generation or error notification.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:NO_MEMORY
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getLastCallFailCauseResponse(
            in RadioResponseInfo info, in LastCallFailCauseInfo failCauseinfo);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param enable true for "mute enabled" and false for "mute disabled"
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SS_MODIFIED_TO_DIAL
     *   RadioError:SS_MODIFIED_TO_USSD
     *   RadioError:SS_MODIFIED_TO_SS
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getMuteResponse(in RadioResponseInfo info, in boolean enable);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param enable false for Standard Privacy Mode (Public Long Code Mask)
     *        true for Enhanced Privacy Mode (Private Long Code Mask)
     *
     * Valid errors:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getPreferredVoicePrivacyResponse(in RadioResponseInfo info, in boolean enable);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param mode TtyMode
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getTtyModeResponse(in RadioResponseInfo info, in TtyMode mode);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void handleStkCallSetupRequestFromSimResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void hangupConnectionResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:NO_RESOURCES
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:SYSTEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void hangupForegroundResumeBackgroundResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:NO_RESOURCES
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:SYSTEM_ERR
     *   RadioError:CANCELLED
     */
    void hangupWaitingOrBackgroundResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param enable true for "vonr enabled" and false for "vonr disabled"
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.ims is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     */
    void isVoNrEnabledResponse(in RadioResponseInfo info, in boolean enable);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:INVALID_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void rejectCallResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:INVALID_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:OPERATION_NOT_ALLOWED
     */
    void sendBurstDtmfResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:OPERATION_NOT_ALLOWED
     */
    void sendCdmaFeatureCodeResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void sendDtmfResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:USSD_MODIFIED_TO_DIAL
     *   RadioError:USSD_MODIFIED_TO_SS
     *   RadioError:USSD_MODIFIED_TO_USSD
     *   RadioError:SIM_BUSY
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:ABORTED
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void sendUssdResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:CANCELLED
     */
    void separateConnectionResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SS_MODIFIED_TO_DIAL
     *   RadioError:SS_MODIFIED_TO_USSD
     *   RadioError:SS_MODIFIED_TO_SS
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_STATE
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setCallForwardResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SS_MODIFIED_TO_DIAL
     *   RadioError:SS_MODIFIED_TO_USSD
     *   RadioError:SS_MODIFIED_TO_SS
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_STATE
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setCallWaitingResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SS_MODIFIED_TO_DIAL
     *   RadioError:SS_MODIFIED_TO_USSD
     *   RadioError:SS_MODIFIED_TO_SS
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setClirResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setMuteResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setPreferredVoicePrivacyResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setTtyModeResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     */
    void setVoNrEnabledResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:SYSTEM_ERR
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void startDtmfResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:NO_MEMORY
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void stopDtmfResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_CALL_ID
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void switchWaitingOrHoldingAndActiveResponse(in RadioResponseInfo info);
}
