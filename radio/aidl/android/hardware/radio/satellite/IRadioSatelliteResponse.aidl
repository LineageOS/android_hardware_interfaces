/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.radio.satellite;

import android.hardware.radio.RadioResponseInfo;
import android.hardware.radio.satellite.NTRadioTechnology;
import android.hardware.radio.satellite.SatelliteCapabilities;
import android.hardware.radio.satellite.SatelliteMode;

/**
 * Interface declaring response functions to solicited radio requests for satellite APIs.
 */
@VintfStability
oneway interface IRadioSatelliteResponse {
    /**
     * Acknowledge the receipt of radio request sent to the vendor. This must be sent only for
     * radio request which take long time to respond. For more details, refer
     * https://source.android.com/devices/tech/connect/ril.html
     *
     * @param serial Serial no. of the request whose acknowledgement is sent.
     */
    void acknowledgeRequest(in int serial);

    /**
     * Response of the request addAllowedSatelliteContacts.
     *
     * @param info Response info struct containing serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:ABORTED
     *   RadioError:ACCESS_BARRED
     *   RadioError:CANCELLED
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_CONTACT
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:NETWORK_REJECT
     *   RadioError:NETWORK_TIMEOUT
     *   RadioError:NO_MEMORY
     *   RadioError:NO_NETWORK_FOUND
     *   RadioError:NO_RESOURCES
     *   RadioError:NO_SATELLITE_SIGNAL
     *   RadioError:NO_SUBSCRIPTION
     *   RadioError:NOT_SUFFICIENT_ACCOUNT_BALANCE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SIM_ABSENT
     *   RadioError:SIM_BUSY
     *   RadioError:SIM_ERR
     *   RadioError:SIM_FULL
     *   RadioError:SYSTEM_ERR
     *   RadioError:UNIDENTIFIED_SUBSCRIBER
     */
    void addAllowedSatelliteContactsResponse(in RadioResponseInfo info);

    /**
     * Response of the request getCapabilities.
     *
     * @param info Response info struct containing serial no. and error
     * @param capabilities List of capabilities that the satellite modem supports.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     */
    void getCapabilitiesResponse(in RadioResponseInfo info, in SatelliteCapabilities capabilities);

    /**
     * Response of the request getMaxCharactersPerTextMessage.
     *
     * @param info Response info struct containing serial no. and error
     * @param charLimit Maximum number of characters in a text message that can be sent.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     */
    void getMaxCharactersPerTextMessageResponse(in RadioResponseInfo info, in int charLimit);

    /**
     * Response of the request getPendingMessages.
     *
     * @param info Response info struct containing serial no. and error
     * @param messages List of pending messages received.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:ABORTED
     *   RadioError:ACCESS_BARRED
     *   RadioError:BLOCKED_DUE_TO_CALL
     *   RadioError:CANCELLED
     *   RadioError:ENCODING_ERR
     *   RadioError:ENCODING_NOT_SUPPORTED
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:NETWORK_REJECT
     *   RadioError:NETWORK_TIMEOUT
     *   RadioError:NO_MEMORY
     *   RadioError:NO_NETWORK_FOUND
     *   RadioError:NO_RESOURCES
     *   RadioError:NO_SMS_TO_ACK
     *   RadioError:NO_SATELLITE_SIGNAL
     *   RadioError:NO_SUBSCRIPTION
     *   RadioError:NOT_SUFFICIENT_ACCOUNT_BALANCE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SIM_ABSENT
     *   RadioError:SIM_BUSY
     *   RadioError:SIM_ERR
     *   RadioError:SIM_FULL
     *   RadioError:SIMULTANEOUS_SMS_AND_CALL_NOT_ALLOWED
     *   RadioError:SYSTEM_ERR
     *   RadioError:SWITCHED_FROM_SATELLITE_TO_TERRESTRIAL
     */
    void getPendingMessagesResponse(in RadioResponseInfo info, in String[] messages);

    /**
     * Response of the request getPowerSate.
     *
     * @param info Response info struct containing serial no. and error
     * @param on True means the modem is ON.
     *           False means the modem is OFF.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     */
    void getPowerStateResponse(in RadioResponseInfo info, in boolean on);

    /**
     * Response of the request getSatelliteMode.
     *
     * @param info Response info struct containing serial no. and error
     * @param mode Current Mode of the satellite modem.
     * @param technology The current technology of the satellite modem.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     */
    void getSatelliteModeResponse(
            in RadioResponseInfo info, in SatelliteMode mode, in NTRadioTechnology technology);

    /**
     * Response of the request getTimeForNextSatelliteVisibility.
     *
     * @param info Response info struct containing serial no. and error
     * @param timeInSeconds The duration in seconds after which the satellite will be visible.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     */
    void getTimeForNextSatelliteVisibilityResponse(in RadioResponseInfo info, in int timeInSeconds);

    /**
     * Response of the request provisionService.
     *
     * @param info Response info struct containing serial no. and error
     * @param provisioned True means the service is provisioned.
     *                    False means the service is not provisioned.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:ABORTED
     *   RadioError:ACCESS_BARRED
     *   RadioError:CANCELLED
     *   RadioError:FEATURE_NOT_SUPPORTED
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:MODEM_INCOMPATIBLE
     *   RadioError:NETWORK_ERR
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:NETWORK_REJECT
     *   RadioError:NETWORK_TIMEOUT
     *   RadioError:NO_MEMORY
     *   RadioError:NO_NETWORK_FOUND
     *   RadioError:NO_RESOURCES
     *   RadioError:NO_SATELLITE_SIGNAL
     *   RadioError:NO_SUBSCRIPTION
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:RADIO_TECHNOLOGY_NOT_SUPPORTED
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SIM_ABSENT
     *   RadioError:SIM_BUSY
     *   RadioError:SIM_ERR
     *   RadioError:SIM_FULL
     *   RadioError:SUBSCRIBER_NOT_AUTHORIZED
     *   RadioError:SYSTEM_ERR
     */
    void provisionServiceResponse(in RadioResponseInfo info, in boolean provisioned);

    /**
     * Response of the request removeAllowedSatelliteContacts.
     *
     * @param info Response info struct containing serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:ABORTED
     *   RadioError:ACCESS_BARRED
     *   RadioError:CANCELLED
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_CONTACT
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:NETWORK_REJECT
     *   RadioError:NETWORK_TIMEOUT
     *   RadioError:NO_MEMORY
     *   RadioError:NO_NETWORK_FOUND
     *   RadioError:NO_RESOURCES
     *   RadioError:NO_SATELLITE_SIGNAL
     *   RadioError:NO_SUBSCRIPTION
     *   RadioError:NOT_SUFFICIENT_ACCOUNT_BALANCE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SIM_ABSENT
     *   RadioError:SIM_BUSY
     *   RadioError:SIM_ERR
     *   RadioError:SIM_FULL
     *   RadioError:SYSTEM_ERR
     *   RadioError:UNIDENTIFIED_SUBSCRIBER
     */
    void removeAllowedSatelliteContactsResponse(in RadioResponseInfo info);

    /**
     * Response of the request sendMessages.
     *
     * @param info Response info struct containing serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:ABORTED
     *   RadioError:ACCESS_BARRED
     *   RadioError:BLOCKED_DUE_TO_CALL
     *   RadioError:CANCELLED
     *   RadioError:ENCODING_ERR
     *   RadioError:ENCODING_NOT_SUPPORTED
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_SMS_FORMAT
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:NETWORK_ERR
     *   RadioError:NETWORK_NOT_READY
     *   RadioError:NETWORK_REJECT
     *   RadioError:NETWORK_TIMEOUT
     *   RadioError:NO_MEMORY
     *   RadioError:NO_NETWORK_FOUND
     *   RadioError:NO_RESOURCES
     *   RadioError:NO_SMS_TO_ACK
     *   RadioError:NO_SATELLITE_SIGNAL
     *   RadioError:NO_SUBSCRIPTION
     *   RadioError:NOT_SUFFICIENT_ACCOUNT_BALANCE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SIM_ABSENT
     *   RadioError:SIM_BUSY
     *   RadioError:SIM_ERR
     *   RadioError:SIM_FULL
     *   RadioError:SIMULTANEOUS_SMS_AND_CALL_NOT_ALLOWED
     *   RadioError:SMS_SEND_FAIL_RETRY
     *   RadioError:SYSTEM_ERR
     *   RadioError:SWITCHED_FROM_SATELLITE_TO_TERRESTRIAL
     *   RadioError:UNIDENTIFIED_SUBSCRIBER
     */
    void sendMessagesResponse(in RadioResponseInfo info);

    /**
     * Response of the request setIndicationFilter.
     *
     * @param info Response info struct containing serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     */
    void setIndicationFilterResponse(in RadioResponseInfo info);

    /**
     * Response of the request setPower.
     *
     * @param info Response info struct containing serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:NO_RF_CALIBRATION_INFO
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:RF_HARDWARE_ISSUE
     *   RadioError:SYSTEM_ERR
     */
    void setPowerResponse(in RadioResponseInfo info);

    /**
     * Response of the request startSendingSatellitePointingInfo.
     *
     * @param info Response info struct containing serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     */
    void startSendingSatellitePointingInfoResponse(in RadioResponseInfo info);

    /**
     * Response of the request stopSendingSatellitePointingInfo.
     *
     * @param info Response info struct containing serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INVALID_MODEM_STATE
     *   RadioError:INVALID_SIM_STATE
     *   RadioError:INVALID_STATE
     *   RadioError:MODEM_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:REQUEST_RATE_LIMITED
     *   RadioError:SYSTEM_ERR
     */
    void stopSendingSatellitePointingInfoResponse(in RadioResponseInfo info);
}
