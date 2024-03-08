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

package android.hardware.radio.data;

import android.hardware.radio.RadioResponseInfo;
import android.hardware.radio.data.KeepaliveStatus;
import android.hardware.radio.data.SetupDataCallResult;
import android.hardware.radio.data.SlicingConfig;

/**
 * Interface declaring response functions to solicited radio requests for data APIs.
 * @hide
 */
@VintfStability
oneway interface IRadioDataResponse {
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.data is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES- Indicates that no pdu session ids are available
     */
    void allocatePduSessionIdResponse(in RadioResponseInfo info, in int id);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param dcResponse Attributes of data call
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.ims is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
     *   RadioError:INVALID_CALL_ID
     */
    void cancelHandoverResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.data is not defined
     *   RadioError:NONE indicates success. Any other error will remove the network from the list.
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
     * @param dcResponse List of SetupDataCallResult
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.data is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:SIM_ABSENT
     */
    void getDataCallListResponse(in RadioResponseInfo info, in SetupDataCallResult[] dcResponse);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param slicingConfig Current slicing configuration
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.data is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     */
    void getSlicingConfigResponse(in RadioResponseInfo info, in SlicingConfig slicingConfig);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.ims is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
     */
    void releasePduSessionIdResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.data is not defined
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
     */
    void setDataAllowedResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.data is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SUBSCRIPTION_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_MEMORY
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     *   RadioError:SIM_ABSENT
     */
    void setDataProfileResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     *  Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.data is not defined
     *  RadioError:NONE
     *  RadioError:RADIO_NOT_AVAILABLE
     *  RadioError:MODEM_ERR
     *  RadioError:INVALID_ARGUMENTS
     */
    void setDataThrottlingResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.data is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:SUBSCRIPTION_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:SYSTEM_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     *   RadioError:NOT_PROVISIONED
     *   RadioError:NO_RESOURCES
     *   RadioError:CANCELLED
     */
    void setInitialAttachApnResponse(in RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param dcResponse SetupDataCallResult
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.data is not defined
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.ims is not defined
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:NO_RESOURCES
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
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.data is not defined
     *   RadioError:NONE
     *   RadioError:NO_RESOURCES
     *   RadioError:INVALID_ARGUMENTS
     */
    void startKeepaliveResponse(in RadioResponseInfo info, in KeepaliveStatus status);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:REQUEST_NOT_SUPPORTED when android.hardware.telephony.data is not defined
     *   RadioError:NONE
     *   RadioError:INVALID_ARGUMENTS
     */
    void stopKeepaliveResponse(in RadioResponseInfo info);
}
