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

import android.hardware.radio.RadioResponseInfo;
import android.hardware.radio.sim.CardStatus;
import android.hardware.radio.sim.CarrierRestrictions;
import android.hardware.radio.sim.CdmaSubscriptionSource;
import android.hardware.radio.sim.IccIoResult;
import android.hardware.radio.sim.PersoSubstate;
import android.hardware.radio.sim.PhonebookCapacity;
import android.hardware.radio.sim.SimLockMultiSimPolicy;

/**
 * Interface declaring response functions to solicited radio requests for SIM APIs.
 * @hide
 */
@VintfStability
oneway interface IRadioSimResponse {
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
     * @param enabled whether Uicc applications are enabled.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:SIM_ABSENT
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void areUiccApplicationsEnabledResponse(in RadioResponseInfo info, in boolean enabled);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param remainingRetries Number of retries remaining, must be equal to -1 if unknown.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:PASSWORD_INCORRECT (old PIN2 is invalid)
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:SIM_PUK2
     */
    void changeIccPin2ForAppResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param remainingRetries Number of retries remaining, must be equal to -1 if unknown.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:PASSWORD_INCORRECT
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     */
    void changeIccPinForAppResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:SIM_ABSENT
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:BUSY
     */
    void enableUiccApplicationsResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param carriers Carrier restriction information.
     * @param multiSimPolicy Policy used for devices with multiple SIM cards.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     */
    void getAllowedCarriersResponse(in RadioResponseInfo info, in CarrierRestrictions carriers,
            in SimLockMultiSimPolicy multiSimPolicy);

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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SUBSCRIPTION_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:NOT_PROVISIONED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void getCdmaSubscriptionResponse(in RadioResponseInfo info, in String mdn, in String hSid,
            in String hNid, in String min, in String prl);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param source CDMA subscription source
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SUBSCRIPTION_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void getCdmaSubscriptionSourceResponse(
            in RadioResponseInfo info, in CdmaSubscriptionSource source);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param response 0 is the TS 27.007 service class bit vector of services for which the
     *        specified barring facility is active. "0" means "disabled for all"
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
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
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getFacilityLockForAppResponse(in RadioResponseInfo info, in int response);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param cardStatus ICC card status as defined by CardStatus
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
     */
    void getIccCardStatusResponse(in RadioResponseInfo info, in CardStatus cardStatus);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param imsi String containing the IMSI
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:SIM_ERR
     */
    void getImsiForAppResponse(in RadioResponseInfo info, in String imsi);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param capacity Response capacity enum indicating response processing status
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     */
    void getSimPhonebookCapacityResponse(in RadioResponseInfo info, in PhonebookCapacity capacity);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     */
    void getSimPhonebookRecordsResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *
     * @deprecated use iccCloseLogicalChannelWithSessionInfoResponse instead.
     */
    void iccCloseLogicalChannelResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param iccIo ICC IO operation response
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
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
     */
    void iccIoForAppResponse(in RadioResponseInfo info, in IccIoResult iccIo);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param channelId session id of the logical channel.
     * @param selectResponse Contains the select response for the open channel command with one
     *        byte per integer
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
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
     */
    void iccOpenLogicalChannelResponse(
            in RadioResponseInfo info, in int channelId, in byte[] selectResponse);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param result IccIoResult
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void iccTransmitApduBasicChannelResponse(in RadioResponseInfo info, in IccIoResult result);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param result IccIoResult
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void iccTransmitApduLogicalChannelResponse(in RadioResponseInfo info, in IccIoResult result);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void reportStkServiceIsRunningResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param result IccIoResult
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:SIM_ERR
     *   RadioError:INVALID_ARGUMENTS
     */
    void requestIccSimAuthenticationResponse(in RadioResponseInfo info, in IccIoResult result);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param commandResponse SAT/USAT response in hexadecimal format string starting with first
     *        byte of response
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
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
     *   RadioError:SIM_ABSENT
     */
    void sendEnvelopeResponse(in RadioResponseInfo info, in String commandResponse);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param iccIo IccIoResult corresponding to ICC IO response
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SIM_BUSY
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void sendEnvelopeWithStatusResponse(in RadioResponseInfo info, in IccIoResult iccIo);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:SIM_ABSENT
     */
    void sendTerminalResponseToSimResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     */
    void setAllowedCarriersResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SIM_ABSENT
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INTERNAL_FAILURE
     */
    void setCarrierInfoForImsiEncryptionResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SIM_ABSENT
     *   RadioError:SUBSCRIPTION_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setCdmaSubscriptionSourceResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param retry 0 is the number of retries remaining, or -1 if unknown
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
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
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setFacilityLockForAppResponse(in RadioResponseInfo info, in int retry);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SUBSCRIPTION_NOT_SUPPORTED
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setUiccSubscriptionResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param remainingRetries Number of retries remaining, must be equal to -1 if unknown.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:PASSWORD_INCORRECT
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:SIM_PUK2
     */
    void supplyIccPin2ForAppResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param remainingRetries Number of retries remaining, must be equal to -1 if unknown.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:PASSWORD_INCORRECT
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     */
    void supplyIccPinForAppResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param remainingRetries Number of retries remaining, must be equal to -1 if unknown.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:PASSWORD_INCORRECT (PUK is invalid)
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     */
    void supplyIccPuk2ForAppResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param remainingRetries Number of retries remaining, must be equal to -1 if unknown.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE (radio resetting)
     *   RadioError:PASSWORD_INCORRECT (PUK is invalid)
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     */
    void supplyIccPukForAppResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param persoType SIM Personalization type
     * @param remainingRetries postiive values indicates number of retries remaining, must be equal
     *        to -1 if number of retries is infinite.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
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
     */
    void supplySimDepersonalizationResponse(
            in RadioResponseInfo info, in PersoSubstate persoType, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param updatedRecordIndex The index of the updated or inserted record in the phonebook and
     *        the minimum value is 1
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:SIM_ERR
     *   RadioError:NO_SUCH_ENTRY
     *   RadioError:NO_RESOURCES
     */
    void updateSimPhonebookRecordsResponse(in RadioResponseInfo info, in int updatedRecordIndex);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.subscription is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void iccCloseLogicalChannelWithSessionInfoResponse(in RadioResponseInfo info);
}
