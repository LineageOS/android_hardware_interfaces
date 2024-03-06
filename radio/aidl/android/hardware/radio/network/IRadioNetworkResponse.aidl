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

package android.hardware.radio.network;

import android.hardware.radio.RadioResponseInfo;
import android.hardware.radio.RadioTechnology;
import android.hardware.radio.RadioTechnologyFamily;
import android.hardware.radio.network.BarringInfo;
import android.hardware.radio.network.CdmaRoamingType;
import android.hardware.radio.network.CellIdentity;
import android.hardware.radio.network.CellInfo;
import android.hardware.radio.network.EmergencyRegResult;
import android.hardware.radio.network.OperatorInfo;
import android.hardware.radio.network.RadioAccessSpecifier;
import android.hardware.radio.network.RadioBandMode;
import android.hardware.radio.network.RegStateResult;
import android.hardware.radio.network.SignalStrength;
import android.hardware.radio.network.UsageSetting;

/**
 * Interface declaring response functions to solicited radio requests for network APIs.
 * @hide
 */
@VintfStability
oneway interface IRadioNetworkResponse {
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
     * @param networkTypeBitmap a 32-bit bitmap of RadioAccessFamily.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:MODE_NOT_SUPPORTED
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:NO_RESOURCES
     */
    void getAllowedNetworkTypesBitmapResponse(in RadioResponseInfo info, in int networkTypeBitmap);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param bandModes List of RadioBandMode listing supported modes
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getAvailableBandModesResponse(in RadioResponseInfo info, in RadioBandMode[] bandModes);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param networkInfos List of network operator information as OperatorInfos
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:ABORTED
     *   RadioError:DEVICE_IN_USE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:MODEM_ERR
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     */
    void getBarringInfoResponse(
            in RadioResponseInfo info, in CellIdentity cellIdentity, in BarringInfo[] barringInfos);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param type CdmaRoamingType
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
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void getCdmaRoamingPreferenceResponse(in RadioResponseInfo info, in CdmaRoamingType type);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param cellInfo List of current cell information known to radio
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void getCellInfoListResponse(in RadioResponseInfo info, in CellInfo[] cellInfo);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param dataRegResponse Current data registration response as defined by RegStateResult
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NOT_PROVISIONED
     */
    void getDataRegistrationStateResponse(
            in RadioResponseInfo info, in RegStateResult dataRegResponse);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param isRegistered false = not registered, true = registered
     * @param ratFamily RadioTechnologyFamily. This value is valid only if isRegistered is true.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.ims is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:INVALID_MODEM_STATE
     *
     * @deprecated Deprecated starting from Android U.
     */
    void getImsRegistrationStateResponse(
            in RadioResponseInfo info, in boolean isRegistered, in RadioTechnologyFamily ratFamily);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param selection false for automatic selection, true for manual selection
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getOperatorResponse(
            in RadioResponseInfo info, in String longName, in String shortName, in String numeric);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param signalStrength Current signal strength
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void getSignalStrengthResponse(in RadioResponseInfo info, in SignalStrength signalStrength);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param specifiers List of RadioAccessSpecifiers that are scanned.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     */
    void getSystemSelectionChannelsResponse(
            in RadioResponseInfo info, in RadioAccessSpecifier[] specifiers);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param rat Current voice RAT
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void getVoiceRadioTechnologyResponse(in RadioResponseInfo info, in RadioTechnology rat);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param voiceRegResponse Current Voice registration response as defined by RegStateResult
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void getVoiceRegistrationStateResponse(
            in RadioResponseInfo info, in RegStateResult voiceRegResponse);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param isEnabled Indicates whether NR dual connectivity is enabled or not, True if enabled
     *        else false.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void isNrDualConnectivityEnabledResponse(in RadioResponseInfo info, in boolean isEnabled);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:MODE_NOT_SUPPORTED
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:NO_RESOURCES
     */
    void setAllowedNetworkTypesBitmapResponse(in RadioResponseInfo info);

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
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setBandModeResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
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
    void setBarringPasswordResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.cdma is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setCellInfoListRateResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void setLocationUpdatesResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:ILLEGAL_SIM_OR_ME
     *   RadioError:OPERATION_NOT_ALLOWED
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:MODEM_ERR
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_STATE
     */
    void setNrDualConnectivityStateResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:RADIO_NOT_AVAILABLE
     */
    void setSignalStrengthReportingCriteriaResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.calling is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:SIM_BUSY
     *   RadioError:NO_MEMORY
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void setSuppServiceNotificationsResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
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
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void supplyNetworkDepersonalizationResponse(in RadioResponseInfo info, in int remainingRetries);

    /**
     * @param info Response info struct containing response type, serial no. and error.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_STATE
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:INTERNAL_ERR
     *   RadioError:SIM_ABSENT
     */
    oneway void setUsageSettingResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error.
     * @param usageSetting the usage setting for the current SIM.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INVALID_STATE
     *   RadioError:INTERNAL_ERR
     *   RadioError:SIM_ABSENT
     */
    oneway void getUsageSettingResponse(in RadioResponseInfo info, in UsageSetting usageSetting);

    /**
     * Response of setEmergencyMode.
     * This is an optional API.
     *
     * @param info Response info struct containing response type, serial no. and error.
     * @param regState the current registration state of the modem.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     */
    void setEmergencyModeResponse(in RadioResponseInfo info, in EmergencyRegResult regState);

    /**
     * Response of triggerEmergencyNetworkScan.
     * This is an optional API.
     *
     * @param info Response info struct containing response type, serial no. and error.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     */
    void triggerEmergencyNetworkScanResponse(in RadioResponseInfo info);

    /**
     * Response of exitEmergencyMode.
     * This is an optional API.
     *
     * @param info Response info struct containing response type, serial no. and error.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:MODEM_ERR
     */
    void exitEmergencyModeResponse(in RadioResponseInfo info);

    /**
     * Response of cancelEmergencyNetworkScan.
     * This is an optional API.
     *
     * @param info Response info struct containing response type, serial no. and error.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:MODEM_ERR
     */
    void cancelEmergencyNetworkScanResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:MODEM_ERR
     */
    void setNullCipherAndIntegrityEnabledResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param enabled the last known state of null ciphering and integrity algorithms
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:MODEM_ERR
     */
    void isNullCipherAndIntegrityEnabledResponse(in RadioResponseInfo info, in boolean isEnabled);

    /**
     * Response of isN1ModeEnabled.
     * This is an optional API.
     *
     * @param info Response info struct containing response type, serial no. and error.
     * @param isEnabled Indicates whether N1 mode is enabled or not.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    void isN1ModeEnabledResponse(in RadioResponseInfo info, boolean isEnabled);

    /**
     * Response of setN1ModeEnabled.
     * This is an optional API.
     *
     * @param info Response info struct containing response type, serial no. and error.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_STATE
     */
    void setN1ModeEnabledResponse(in RadioResponseInfo info);

    /**
     * Response of isCellularIdentifierTransparencyEnabled.
     *
     * @param info Response info struct containing response type, serial no. and error.
     * @param isEnabled Indicates whether cellular identifier transparency is enabled or not.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void isCellularIdentifierTransparencyEnabledResponse(
            in RadioResponseInfo info, boolean isEnabled);

    /**
     * Response of setCellularIdentifierTransparencyEnabled.
     *
     * @param info Response info struct containing response type, serial no. and error.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_STATE
     */
    void setCellularIdentifierTransparencyEnabledResponse(in RadioResponseInfo info);

    /**
     * Response of setSecurityAlgorithmsUpdatedEnabled.
     *
     * @param info Response info struct containing response type, serial no. and error.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_STATE
     */
    void setSecurityAlgorithmsUpdatedEnabledResponse(in RadioResponseInfo info);

    /**
     * Response of isSecurityAlgorithmsUpdatedEnabled.
     *
     * @param info Response info struct containing response type, serial no. and error.
     * @param isEnabled Indicates whether cellular ciphering transparency is enabled or not.
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.radio.access is not
     *                                    defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    void isSecurityAlgorithmsUpdatedEnabledResponse(
            in RadioResponseInfo info, in boolean isEnabled);
}
