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

import android.hardware.radio.ActivityStatsInfo;
import android.hardware.radio.BarringInfo;
import android.hardware.radio.Call;
import android.hardware.radio.CallForwardInfo;
import android.hardware.radio.CardStatus;
import android.hardware.radio.CarrierRestrictions;
import android.hardware.radio.CdmaBroadcastSmsConfigInfo;
import android.hardware.radio.CdmaRoamingType;
import android.hardware.radio.CdmaSubscriptionSource;
import android.hardware.radio.CellIdentity;
import android.hardware.radio.CellInfo;
import android.hardware.radio.ClipStatus;
import android.hardware.radio.DataRegStateResult;
import android.hardware.radio.GsmBroadcastSmsConfigInfo;
import android.hardware.radio.HardwareConfig;
import android.hardware.radio.IccIoResult;
import android.hardware.radio.KeepaliveStatus;
import android.hardware.radio.LastCallFailCauseInfo;
import android.hardware.radio.LceDataInfo;
import android.hardware.radio.LceStatusInfo;
import android.hardware.radio.NeighboringCell;
import android.hardware.radio.OperatorInfo;
import android.hardware.radio.PersoSubstate;
import android.hardware.radio.PhonebookCapacity;
import android.hardware.radio.PreferredNetworkType;
import android.hardware.radio.RadioAccessFamily;
import android.hardware.radio.RadioAccessSpecifier;
import android.hardware.radio.RadioBandMode;
import android.hardware.radio.RadioCapability;
import android.hardware.radio.RadioResponseInfo;
import android.hardware.radio.RadioTechnology;
import android.hardware.radio.RadioTechnologyFamily;
import android.hardware.radio.RegStateResult;
import android.hardware.radio.SendSmsResult;
import android.hardware.radio.SetupDataCallResult;
import android.hardware.radio.SignalStrength;
import android.hardware.radio.SimLockMultiSimPolicy;
import android.hardware.radio.SlicingConfig;
import android.hardware.radio.TtyMode;
import android.hardware.radio.VoiceRegStateResult;

/**
 * Interface declaring response functions to solicited radio requests.
 */
@VintfStability
oneway interface IRadioResponse {
    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void acceptCallResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void acknowledgeIncomingGsmSmsWithPduResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_SMS_TO_ACK
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void acknowledgeLastIncomingCdmaSmsResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void acknowledgeLastIncomingGsmSmsResponse(in RadioResponseInfo info);

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
     * @param id The allocated id. On an error, this is set to 0.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES- Indicates that no pdu session ids are available
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void allocatePduSessionIdResponse(in RadioResponseInfo info, in int id);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param enabled whether Uicc applications are enabled.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:SIM_ABSENT
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void areUiccApplicationsEnabledResponse(in RadioResponseInfo info, in boolean enabled);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param dcResponse Attributes of data call
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_CALL_ID
     */
    void cancelHandoverResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void cancelPendingUssdResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param remainingRetries Number of retries remaining, must be equal to -1 if unknown.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:PASSWORD_INCORRECT (old PIN2 is invalid)
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:SIM_PUK2
     */
    void changeIccPin2ForAppResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param remainingRetries Number of retries remaining, must be equal to -1 if unknown.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:PASSWORD_INCORRECT
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void changeIccPinForAppResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void conferenceResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED may be returned when HAL 1.2 or higher is supported.
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_CALL_ID
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void deactivateDataCallResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NO_SUCH_ENTRY
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:SIM_ABSENT
     */
    void deleteSmsOnRuimResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SIM_FULL
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NO_SUCH_ENTRY
     *   RadioError:INTERNAL_ERR
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:SIM_ABSENT
     */
    void deleteSmsOnSimResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:CANCELLED
     */
    void dialResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_STATE: this is for the case that the API is called in a single-sim
     *              mode, or when there is only one modem available, as this API should only
     *              be called in multi sim status.
     */
    void enableModemResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:SIM_ABSENT
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:BUSY
     */
    void enableUiccApplicationsResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NO_ALLOWED
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void exitEmergencyCallbackModeResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void explicitCallTransferResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param carriers Carrier restriction information.
     * @param multiSimPolicy Policy used for devices with multiple SIM cards.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void getAllowedCarriersResponse(in RadioResponseInfo info, in CarrierRestrictions carriers,
            in SimLockMultiSimPolicy multiSimPolicy);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param networkTypeBitmap a 32-bit bitmap of RadioAccessFamily.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:MODE_NOT_SUPPORTED
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     */
    void getAllowedNetworkTypesBitmapResponse(
            in RadioResponseInfo info, in RadioAccessFamily networkTypeBitmap);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param bandModes List of RadioBandMode listing supported modes
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getAvailableBandModesResponse(in RadioResponseInfo info, in RadioBandMode[] bandModes);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param networkInfos List of network operator information as OperatorInfos defined in
     *        types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:ABORTED
     *   RadioError:DEVICE_IN_USE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:CANCELLED
     *   RadioError:NO_RESOURCES
     *   RadioError:INTERNAL_ERR
     */
    void getAvailableNetworksResponse(in RadioResponseInfo info, in OperatorInfo[] networkInfos);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param cellIdentity CellIdentity for the barring infos.
     * @param barringInfos a vector of barring info for all barring service types
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     */
    void getBarringInfoResponse(
            in RadioResponseInfo info, in CellIdentity cellIdentity, in BarringInfo[] barringInfos);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param version string containing version string for log reporting
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:EMPTY_RECORD
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NOT_PROVISIONED
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getBasebandVersionResponse(in RadioResponseInfo info, in String version);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param mdn MDN if CDMA subscription is available
     * @param hSid is a comma separated list of H_SID (Home SID) if CDMA subscription is available,
     *        in decimal format
     * @param hNid is a comma separated list of H_NID (Home NID) if CDMA subscription is available,
     *        in decimal format
     * @param min MIN (10 digits, MIN2+MIN1) if CDMA subscription is available
     * @param prl PRL version if CDMA subscription is available
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SUBSCRIPTION_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:NOT_PROVISIONED
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void getCDMASubscriptionResponse(in RadioResponseInfo info, in String mdn, in String hSid,
            in String hNid, in String min, in String prl);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param callForwardInfos points to a vector of CallForwardInfo, one for each distinct
     *        registered phone number. For example, if data is forwarded to +18005551212 and voice
     *        is forwarded to +18005559999, then two separate CallForwardInfo's must be returned.
     *        However, if both data and voice are forwarded to +18005551212, then a single
     *        CallForwardInfo must be returned with the service class set to "data + voice = 3".
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getCallWaitingResponse(in RadioResponseInfo info, in boolean enable, in int serviceClass);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param configs Vector of CDMA Broadcast SMS configs.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void getCdmaBroadcastConfigResponse(
            in RadioResponseInfo info, in CdmaBroadcastSmsConfigInfo[] configs);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param type CdmaRoamingType defined in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void getCdmaRoamingPreferenceResponse(in RadioResponseInfo info, in CdmaRoamingType type);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param source CDMA subscription source
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SUBSCRIPTION_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:SIM_ABSENT
     */
    void getCdmaSubscriptionSourceResponse(
            in RadioResponseInfo info, in CdmaSubscriptionSource source);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param cellInfo List of current cell information known to radio
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void getCellInfoListResponse(in RadioResponseInfo info, in CellInfo[] cellInfo);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param status indicates CLIP status
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:REQUEST_NOT_SUPPORTED
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
     *   RadioError:REQUEST_NOT_SUPPORTED
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
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getCurrentCallsResponse(in RadioResponseInfo info, in Call[] calls);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param dcResponse List of SetupDataCallResult as defined in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:SIM_ABSENT
     */
    void getDataCallListResponse(in RadioResponseInfo info, in SetupDataCallResult[] dcResponse);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param dataRegResponse Current Data registration response as defined by RegStateResult in
     *        types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NOT_PROVISIONED
     */
    void getDataRegistrationStateResponse(
            in RadioResponseInfo info, in RegStateResult dataRegResponse);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param imei IMEI if GSM subscription is available
     * @param imeisv IMEISV if GSM subscription is available
     * @param esn ESN if CDMA subscription is available
     * @param meid MEID if CDMA subscription is available
     *
     * If a empty string value is returned for any of the device id, it means that there was error
     * accessing the device.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:NOT_PROVISIONED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void getDeviceIdentityResponse(in RadioResponseInfo info, in String imei, in String imeisv,
            in String esn, in String meid);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param response 0 is the TS 27.007 service class bit vector of services for which the
     *        specified barring facility is active. "0" means "disabled for all"
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SS_MODIFIED_TO_DIAL
     *   RadioError:SS_MODIFIED_TO_USSD
     *   RadioError:SS_MODIFIED_TO_SS
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getFacilityLockForAppResponse(in RadioResponseInfo info, in int response);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param configs Vector of GSM/WCDMA Cell broadcast configs
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void getGsmBroadcastConfigResponse(
            in RadioResponseInfo info, in GsmBroadcastSmsConfigInfo[] configs);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param config Array of HardwareConfig of the radio.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void getHardwareConfigResponse(in RadioResponseInfo info, in HardwareConfig[] config);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param imsi String containing the IMSI
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:SIM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void getIMSIForAppResponse(in RadioResponseInfo info, in String imsi);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param cardStatus ICC card status as defined by CardStatus in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void getIccCardStatusResponse(in RadioResponseInfo info, in CardStatus cardStatus);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param isRegistered false = not registered, true = registered
     * @param ratFamily RadioTechnologyFamily as defined in types.hal. This value is valid only if
     *        isRegistered is true.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void getImsRegistrationStateResponse(
            in RadioResponseInfo info, in boolean isRegistered, in RadioTechnologyFamily ratFamily);

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
     *   RadioError:NONE
     *   RadioError:NO_MEMORY
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getLastCallFailCauseResponse(
            in RadioResponseInfo info, in LastCallFailCauseInfo failCauseinfo);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param activityInfo modem activity information
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NOT_PROVISIONED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void getModemActivityInfoResponse(in RadioResponseInfo info, in ActivityStatsInfo activityInfo);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:MODEM_ERR
     */
    void getModemStackStatusResponse(in RadioResponseInfo info, in boolean isEnabled);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param enable true for "mute enabled" and false for "mute disabled"
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getMuteResponse(in RadioResponseInfo info, in boolean enable);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param cells Vector of neighboring radio cell
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NO_NETWORK_FOUND
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getNeighboringCidsResponse(in RadioResponseInfo info, in NeighboringCell[] cells);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param selection false for automatic selection, true for manual selection
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getNetworkSelectionModeResponse(in RadioResponseInfo info, in boolean manual);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param longName is long alpha ONS or EONS or empty string if unregistered
     * @param shortName is short alpha ONS or EONS or empty string if unregistered
     * @param numeric is 5 or 6 digit numeric code (MCC + MNC) or empty string if unregistered
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getOperatorResponse(
            in RadioResponseInfo info, in String longName, in String shortName, in String numeric);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param networkTypeBitmap a 32-bit bitmap of RadioAccessFamily.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     */
    void getPreferredNetworkTypeBitmapResponse(
            in RadioResponseInfo info, in RadioAccessFamily networkTypeBitmap);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param nwType RadioPreferredNetworkType defined in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getPreferredNetworkTypeResponse(in RadioResponseInfo info, in PreferredNetworkType nwType);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param enable false for Standard Privacy Mode (Public Long Code Mask)
     *        true for Enhanced Privacy Mode (Private Long Code Mask)
     *
     * Valid errors:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getPreferredVoicePrivacyResponse(in RadioResponseInfo info, in boolean enable);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param rc Radio capability as defined by RadioCapability in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INVALID_STATE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getRadioCapabilityResponse(in RadioResponseInfo info, in RadioCapability rc);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param signalStrength Current signal strength
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void getSignalStrengthResponse(in RadioResponseInfo info, in SignalStrength signalStrength);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param capacity Response capacity enum indicating response processing status
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     * REQUEST_NOT_SUPPORTED may only be returned on devices that don't support this API,
     * indicated by the HAL capability CAPABILITY_SIM_PHONEBOOK_IN_MODEM.
     */
    void getSimPhonebookCapacityResponse(in RadioResponseInfo info, in PhonebookCapacity capacity);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     * REQUEST_NOT_SUPPORTED may only be returned on devices that don't support this API,
     * indicated by the HAL capability CAPABILITY_SIM_PHONEBOOK_IN_MODEM.
     */
    void getSimPhonebookRecordsResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param slicingConfig Current slicing configuration
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     */
    void getSlicingConfigResponse(in RadioResponseInfo info, in SlicingConfig slicingConfig);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param smsc Short Message Service Center address on the device
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NOT_PROVISIONED
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void getSmscAddressResponse(in RadioResponseInfo info, in String smsc);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param specifiers List of RadioAccessSpecifiers that are scanned.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     */
    void getSystemSelectionChannelsResponse(
            in RadioResponseInfo info, in RadioAccessSpecifier[] specifiers);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param mode TtyMode
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getTTYModeResponse(in RadioResponseInfo info, in TtyMode mode);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param rat Current voice RAT
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void getVoiceRadioTechnologyResponse(in RadioResponseInfo info, in RadioTechnology rat);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param voiceRegResponse Current Voice registration response as defined by RegStateResult
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void getVoiceRegistrationStateResponse(
            in RadioResponseInfo info, in RegStateResult voiceRegResponse);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:SIM_ABSENT
     */
    void handleStkCallSetupRequestFromSimResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void hangupConnectionResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void hangupForegroundResumeBackgroundResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:CANCELLED
     */
    void hangupWaitingOrBackgroundResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void iccCloseLogicalChannelResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param iccIo ICC io operation response as defined by IccIoResult in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SIM_PIN2
     *   RadioError:SIM_PUK2
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:SIM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void iccIOForAppResponse(in RadioResponseInfo info, in IccIoResult iccIo);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param channelId session id of the logical channel.
     * @param selectResponse Contains the select response for the open channel command with one
     *        byte per integer
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:MISSING_RESOURCE
     *   RadioError:NO_SUCH_ELEMENT
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ERR
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:MISSING_RESOURCE
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void iccOpenLogicalChannelResponse(
            in RadioResponseInfo info, in int channelId, in byte[] selectResponse);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param result IccIoResult as defined in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void iccTransmitApduBasicChannelResponse(in RadioResponseInfo info, in IccIoResult result);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param result IccIoResult as defined in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void iccTransmitApduLogicalChannelResponse(in RadioResponseInfo info, in IccIoResult result);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param isEnabled Indicates whether NR dual connectivity is enabled or not, True if enabled
     *        else false.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void isNrDualConnectivityEnabledResponse(in RadioResponseInfo info, in boolean isEnabled);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param result string containing the contents of the NV item
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void nvReadItemResponse(in RadioResponseInfo info, in String result);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void nvResetConfigResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void nvWriteCdmaPrlResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void nvWriteItemResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param lceInfo LceDataInfo indicating LCE data
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED may be returned when HAL 1.2 or higher is supported.
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:LCE_NOT_SUPPORTED
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void pullLceDataResponse(in RadioResponseInfo info, in LceDataInfo lceInfo);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void rejectCallResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void releasePduSessionIdResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_STATE
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void reportSmsMemoryStatusResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void reportStkServiceIsRunningResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param result IccIoResult as defined in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:SIM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void requestIccSimAuthenticationResponse(in RadioResponseInfo info, in IccIoResult result);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param response response string of the challenge/response algo for ISIM auth in base64 format
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:SIM_ABSENT
     */
    void requestIsimAuthenticationResponse(in RadioResponseInfo info, in String response);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void requestShutdownResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:INVALID_STATE
     *   RadioError:REQUEST_NOT_SUPPORTED
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
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:INVALID_STATE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:OPERATION_NOT_ALLOWED
     */
    void sendCDMAFeatureCodeResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param sms Response to sms sent as defined by SendSmsResult in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SMS_SEND_FAIL_RETRY
     *   RadioError:NETWORK_REJECT
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:SYSTEM_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:ENCODING_ERR
     *   RadioError:INVALID_SMSC_ADDRESS
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     *   RadioError:SIMULTANEOUS_SMS_AND_CALL_NOT_ALLOWED
     *   RadioError:ACCESS_BARRED
     *   RadioError:BLOCKED_DUE_TO_CALL
     */
    void sendCdmaSmsExpectMoreResponse(in RadioResponseInfo info, in SendSmsResult sms);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param sms Sms result struct as defined by SendSmsResult in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:SMS_SEND_FAIL_RETRY
     *   RadioError:NETWORK_REJECT
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:SYSTEM_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:ENCODING_ERR
     *   RadioError:INVALID_SMSC_ADDRESS
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:ENCODING_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     *   RadioError:SIMULTANEOUS_SMS_AND_CALL_NOT_ALLOWED
     *   RadioError:ACCESS_BARRED
     *   RadioError:BLOCKED_DUE_TO_CALL
     */
    void sendCdmaSmsResponse(in RadioResponseInfo info, in SendSmsResult sms);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void sendDeviceStateResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_CALL_ID
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void sendDtmfResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param commandResponse SAT/USAT response in hexadecimal format string starting with first
     *        byte of response
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SIM_BUSY
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:SIM_ABSENT
     */
    void sendEnvelopeResponse(in RadioResponseInfo info, in String commandResponse);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param iccIo IccIoResult as defined in types.hal corresponding to ICC IO response
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SIM_BUSY
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:SIM_ABSENT
     */
    void sendEnvelopeWithStatusResponse(in RadioResponseInfo info, in IccIoResult iccIo);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param sms Response to sms sent as defined by SendSmsResult in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SMS_SEND_FAIL_RETRY
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:NETWORK_REJECT
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:ENCODING_ERR
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void sendImsSmsResponse(in RadioResponseInfo info, in SendSmsResult sms);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param sms Response to sms sent as defined by SendSmsResult in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SMS_SEND_FAIL_RETRY
     *   RadioError:NETWORK_REJECT
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:SYSTEM_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:ENCODING_ERR
     *   RadioError:INVALID_SMSC_ADDRESS
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void sendSMSExpectMoreResponse(in RadioResponseInfo info, in SendSmsResult sms);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param sms Response to sms sent as defined by SendSmsResult in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SMS_SEND_FAIL_RETRY
     *   RadioError:NETWORK_REJECT
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:SYSTEM_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:ENCODING_ERR
     *   RadioError:INVALID_SMSC_ADDRESS
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     *   RadioError:ACCESS_BARRED
     *   RadioError:BLOCKED_DUE_TO_CALL
     */
    void sendSmsExpectMoreResponse(in RadioResponseInfo info, in SendSmsResult sms);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param sms Response to sms sent as defined by SendSmsResult in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SMS_SEND_FAIL_RETRY
     *   RadioError:NETWORK_REJECT
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:SYSTEM_ERR
     *   RadioError:ENCODING_ERR
     *   RadioError:INVALID_SMSC_ADDRESS
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     *   RadioError:ACCESS_BARRED
     *   RadioError:BLOCKED_DUE_TO_CALL
     */
    void sendSmsResponse(in RadioResponseInfo info, in SendSmsResult sms);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:SIM_ABSENT
     */
    void sendTerminalResponseToSimResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void sendUssdResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:CANCELLED
     */
    void separateConnectionResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void setAllowedCarriersResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:MODE_NOT_SUPPORTED
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     */
    void setAllowedNetworkTypesBitmapResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setBandModeResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SS_MODIFIED_TO_DIAL
     *   RadioError:SS_MODIFIED_TO_USSD
     *   RadioError:SS_MODIFIED_TO_SS
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setBarringPasswordResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setCallForwardResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setCallWaitingResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:RIL_E_SUCCESS
     *   RadioError:RIL_E_RADIO_NOT_AVAILABLE
     *   RadioError:SIM_ABSENT
     *   RadioError:RIL_E_REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_INTERNAL_FAILURE
     */
    void setCarrierInfoForImsiEncryptionResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void setCdmaBroadcastActivationResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void setCdmaBroadcastConfigResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void setCdmaRoamingPreferenceResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SIM_ABSENT
     *   RadioError:SUBSCRIPTION_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void setCdmaSubscriptionSourceResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void setCellInfoListRateResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SS_MODIFIED_TO_DIAL
     *   RadioError:SS_MODIFIED_TO_USSD
     *   RadioError:SS_MODIFIED_TO_SS
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setClirResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:DEVICE_IN_USE
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void setDataAllowedResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SUBSCRIPTION_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:SIM_ABSENT
     */
    void setDataProfileResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     *  Valid errors returned:
     *  RadioError:NONE
     *  RadioError:RADIO_NOT_AVAILABLE
     *  RadioError:MODEM_ERR
     *  RadioError:INVALID_ARGUMENTS
     *  RadioError:REQUEST_NOT_SUPPORTED
     */
    void setDataThrottlingResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param retry 0 is the number of retries remaining, or -1 if unknown
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SS_MODIFIED_TO_DIAL
     *   RadioError:SS_MODIFIED_TO_USSD
     *   RadioError:SS_MODIFIED_TO_SS
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_STATE
     *   RadioError:FDN_CHECK_FAILURE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setFacilityLockForAppResponse(in RadioResponseInfo info, in int retry);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void setGsmBroadcastActivationResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void setGsmBroadcastConfigResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     */
    void setIndicationFilterResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SUBSCRIPTION_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NOT_PROVISIONED
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setInitialAttachApnResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void setLinkCapacityReportingCriteriaResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void setLocationUpdatesResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setMuteResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:ILLEGAL_SIM_OR_ME
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *
     * Returns RadioError:ILLEGAL_SIM_OR_ME when the failure is permanent and
     * no retries needed, such as illegal SIM or ME.
     */
    void setNetworkSelectionModeAutomaticResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:ILLEGAL_SIM_OR_ME
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INVALID_STATE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *
     * Returns RadioError:ILLEGAL_SIM_OR_ME when the failure is permanent and
     * no retries needed, such as illegal SIM or ME.
     */
    void setNetworkSelectionModeManualResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_STATE
     */
    void setNrDualConnectivityStateResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:MODE_NOT_SUPPORTED
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     */
    void setPreferredNetworkTypeBitmapResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:MODE_NOT_SUPPORTED
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setPreferredNetworkTypeResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_CALL_ID
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setPreferredVoicePrivacyResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param rc Radio capability as defined by RadioCapability in types.hal used to
     *        feedback return status
     *
     * Valid errors returned:
     *   RadioError:NONE means a unsol radioCapability() will be sent within 30 seconds.
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_STATE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setRadioCapabilityResponse(in RadioResponseInfo info, in RadioCapability rc);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:RF_HARDWARE_ISSUE
     *   RadioError:NO_RF_CALIBRATION_INFO
     */
    void setRadioPowerResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:RADIO_NOT_AVAILABLE
     */
    void setSignalStrengthReportingCriteriaResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:SIM_ERR (indicates a timeout or other issue making the SIM unresponsive)
     */
    void setSimCardPowerResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:MODEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void setSmscAddressResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:SIM_BUSY
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void setSuppServiceNotificationsResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     */
    void setSystemSelectionChannelsResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setTTYModeResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SUBSCRIPTION_NOT_SUPPORTED
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setUiccSubscriptionResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param dcResponse SetupDataCallResult defined in types.hal
     *
     * Valid errors returned:
     *   RadioError:NONE must be returned on both success and failure of setup with the
     *              DataCallResponse.status containing the actual status
     *              For all other errors the DataCallResponse is ignored.
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OP_NOT_ALLOWED_BEFORE_REG_TO_NW
     *   RadioError:OP_NOT_ALLOWED_DURING_VOICE_CALL
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES if the vendor is unable handle due to resources are full.
     *   RadioError:SIM_ABSENT
     */
    void setupDataCallResponse(in RadioResponseInfo info, in SetupDataCallResult dcResponse);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void startDtmfResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_CALL_ID
     */
    void startHandoverResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param status Status object containing a new handle and a current status. The status returned
     *        here may be PENDING to indicate that the radio has not yet processed the keepalive
     *        request.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:NO_RESOURCES
     *   RadioError:INVALID_ARGUMENTS
     */
    void startKeepaliveResponse(in RadioResponseInfo info, in KeepaliveStatus status);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param statusInfo LceStatusInfo indicating LCE status
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED may be returned when HAL 1.2 or higher is supported.
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:LCE_NOT_SUPPORTED
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void startLceServiceResponse(in RadioResponseInfo info, in LceStatusInfo statusInfo);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:DEVICE_IN_USE
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     */
    void startNetworkScanResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     */
    void stopDtmfResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INVALID_ARGUMENTS
     */
    void stopKeepaliveResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param statusInfo LceStatusInfo indicating LCE status
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED may be returned when HAL 1.2 or higher is supported.
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:LCE_NOT_SUPPORTED
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void stopLceServiceResponse(in RadioResponseInfo info, in LceStatusInfo statusInfo);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     */
    void stopNetworkScanResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param remainingRetries Number of retries remaining, must be equal to -1 if unknown.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:PASSWORD_INCORRECT
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:SIM_PUK2
     */
    void supplyIccPin2ForAppResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param remainingRetries Number of retries remaining, must be equal to -1 if unknown.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:PASSWORD_INCORRECT
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void supplyIccPinForAppResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param remainingRetries Number of retries remaining, must be equal to -1 if unknown.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:PASSWORD_INCORRECT (PUK is invalid)
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void supplyIccPuk2ForAppResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param remainingRetries Number of retries remaining, must be equal to -1 if unknown.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:PASSWORD_INCORRECT (PUK is invalid)
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void supplyIccPukForAppResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param remainingRetries Number of retries remaining, must be equal to -1 if unknown.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:PASSWORD_INCORRECT (code is invalid)
     *   RadioError:NO_MEMORY
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:SIM_ABSENT
     */
    void supplyNetworkDepersonalizationResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param persoType SIM Personalisation type
     * @param remainingRetries postiive values indicates number of retries remaining, must be equal
     *        to -1 if number of retries is infinite.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:PASSWORD_INCORRECT (code is invalid)
     *   RadioError:NO_MEMORY
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void supplySimDepersonalizationResponse(
            in RadioResponseInfo info, in PersoSubstate persoType, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
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
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void switchWaitingOrHoldingAndActiveResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param updatedRecordIndex The index of the updated or inserted record in the phonebook and
     *        the minimum value is 1
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:SIM_ERR
     *   RadioError:NO_SUCH_ENTRY
     *   RadioError:NO_RESOURCES
     * REQUEST_NOT_SUPPORTED may only be returned on devices that don't support this API,
     * indicated by the HAL capability CAPABILITY_SIM_PHONEBOOK_IN_MODEM.
     */
    void updateSimPhonebookRecordsResponse(in RadioResponseInfo info, in int updatedRecordIndex);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param index record index where the cmda sms message is stored
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:SIM_FULL
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:ENCODING_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SMSC_ADDRESS
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:SIM_ABSENT
     */
    void writeSmsToRuimResponse(in RadioResponseInfo info, in int index);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param index record index where the message is stored
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SIM_FULL
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:ENCODING_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INVALID_SMSC_ADDRESS
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SYSTEM_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:SIM_ABSENT
     */
    void writeSmsToSimResponse(in RadioResponseInfo info, in int index);
}
