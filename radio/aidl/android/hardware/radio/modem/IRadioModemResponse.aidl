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

import android.hardware.radio.RadioResponseInfo;
import android.hardware.radio.modem.ActivityStatsInfo;
import android.hardware.radio.modem.HardwareConfig;
import android.hardware.radio.modem.ImeiInfo;
import android.hardware.radio.modem.RadioCapability;

/**
 * Interface declaring response functions to solicited radio requests for modem APIs.
 * @hide
 */
@VintfStability
oneway interface IRadioModemResponse {
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony is not defined
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
     * @param version string containing version string for log reporting
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:EMPTY_RECORD
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NOT_PROVISIONED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getBasebandVersionResponse(in RadioResponseInfo info, in String version);

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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony is not defined
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
     * @deprecated use getImeiResponse(RadioResponseInfo responseInfo, ImeiInfo imeiInfo)
     */
    void getDeviceIdentityResponse(in RadioResponseInfo info, in String imei, in String imeisv,
            in String esn, in String meid);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param config Array of HardwareConfig of the radio.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     */
    void getHardwareConfigResponse(in RadioResponseInfo info, in HardwareConfig[] config);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param activityInfo modem activity information
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NOT_PROVISIONED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getModemActivityInfoResponse(in RadioResponseInfo info, in ActivityStatsInfo activityInfo);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:MODEM_ERR
     */
    void getModemStackStatusResponse(in RadioResponseInfo info, in boolean isEnabled);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param rc Radio capability as defined by RadioCapability in types.hal
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:INVALID_STATE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getRadioCapabilityResponse(in RadioResponseInfo info, in RadioCapability rc);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param result string containing the contents of the NV item
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    void nvReadItemResponse(in RadioResponseInfo info, in String result);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *
     * Note: This will be deprecated in favor of a rebootModemResponse API in Android U.
     */
    void nvResetConfigResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    void nvWriteCdmaPrlResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *
     * @deprecated NV APIs are deprecated starting from Android U.
     */
    void nvWriteItemResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void requestShutdownResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void sendDeviceStateResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param rc Radio capability as defined by RadioCapability in types.hal used to
     *        feedback return status
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE means a unsol radioCapability() will be sent within 30 seconds.
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_STATE
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setRadioCapabilityResponse(in RadioResponseInfo info, in RadioCapability rc);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:RF_HARDWARE_ISSUE
     *   RadioError:NO_RF_CALIBRATION_INFO
     */
    void setRadioPowerResponse(in RadioResponseInfo info);

    /**
     * ImeiInfo to encapsulate the IMEI information from modem. When the return error code
     * is {@code RadioError:NONE}, {@code imeiInfo} must be non-null, and a valid IMEITYPE,
     * IMEI and SVN must be filled in {@code imeiInfo}. When the error code is not
     * {@code RadioError:NONE}, {@code imeiInfo} must be {@code null}.
     *
     * @param responseInfo Response info struct containing response type, serial no. and error
     * @param imeiInfo IMEI information
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.gsm is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:MODEM_ERR
     */
    void getImeiResponse(in RadioResponseInfo responseInfo, in @nullable ImeiInfo imeiInfo);
}
