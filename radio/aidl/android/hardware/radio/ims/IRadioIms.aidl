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

package android.hardware.radio.ims;

import android.hardware.radio.ims.ImsRegistration;
import android.hardware.radio.ims.ImsTrafficType;
import android.hardware.radio.ims.IRadioImsIndication;
import android.hardware.radio.ims.IRadioImsResponse;
import android.hardware.radio.ims.SrvccCall;

/**
 * This interface is used by IMS telephony layer to talk to cellular radio.
 * All the functions have minimum one parameter:
 * serial: which corresponds to serial no. of request. Serial numbers must only be memorized for the
 * duration of a method call. If clients provide colliding serials (including passing the same
 * serial to different methods), multiple responses (one for each method call) must still be served.
 * setResponseFunctions must work with IRadioImsResponse and IRadioImsIndication.
 */
@VintfStability
oneway interface IRadioIms {
    /**
     * Provides a list of SRVCC call information to radio.
     *
     * @param srvccCalls the list of calls
     *
     * Response function is IRadioImsResponse.setSrvccCallInfoResponse()
     */
    void setSrvccCallInfo(int serial, in SrvccCall[] srvccCalls);

    /**
     * Update the IMS registration information to the radio.
     *
     * This information shall be used by radio to implement following carrier requirements:
     * 1) Graceful IMS PDN disconnection on cellular when NAS is about to perform detach
     * eg. SIM removal or SIM refresh
     * 2) Block PLMN or RAT based on the IMS registration failure reason
     * 3) Start IMS establishment timers when on IMS registration is started
     *
     * @param serial Serial number of request
     * @param imsRegistration IMS registration information
     *
     * Response function is IRadioImsResponse.updateImsRegistrationInfoResponse()
     */
    void updateImsRegistrationInfo(int serial, in ImsRegistration imsRegistration);

    /**
     * IMS stack notifies the NAS and RRC layers of the radio that the upcoming IMS traffic is
     * for the service mentioned in the ImsTrafficType. If this API is not
     * explicitly invoked and IMS module sends traffic on IMS PDN then the radio
     * shall treat type as background data traffic type.
     * This API shall be used by modem
     *  1. To set the appropriate establishment cause in RRC connection request.
     *  2. To prioritize RF resources in case of DSDS. The service priority is
     * EMERGENCY > EMERGENCY SMS > VOICE > VIDEO > SMS > REGISTRATION > BACKGROUND
     * DATA. The RF shall be prioritized to the subscription which handles higher
     * priority service. When both subscriptions are handling the same type of
     * service then RF shall be prioritized to the voice preferred sub.
     *
     * @param token Token number of the request
     * @param imsTrafficType IMS traffic type like registration, voice, and video
     * @param isStart true when the traffic flow starts, false when traffic flow stops.
     *        false will not be notified for SMS as it's a short traffic.
     *
     * Response function is IRadioImsResponse.notifyImsTrafficResponse()
     */
    void notifyImsTraffic(int serial, int token,
            ImsTrafficType imsTrafficType, boolean isStart);

    /**
     * This API shall check access class barring checks based on ImsTrafficType.
     * In case of access class is allowed then
     * IRadioImsIndication#onAccessAllowed(token) shall be invoked by radio
     * so the IMS stack can transmit the data.
     * In case of access is denied
     * IRadioImsIndication#onConnectionSetupFailure(token, REASON_ACCESS_DENIED)
     * shall be invoked.
     *
     * @param token Token number of the request
     * @param imsTrafficType IMS traffic type like registration, voice
     *        video, SMS, emergency etc
     *
     * Response function is IRadioImsResponse.performAcbCheckResponse()
     */
    void performAcbCheck(int serial, int token, ImsTrafficType imsTrafficType);

    /**
     * Set response functions for IMS radio requests and indications.
     *
     * @param radioImsResponse Object containing response functions
     * @param radioImsIndication Object containing radio indications
     */
    void setResponseFunctions(in IRadioImsResponse radioImsResponse,
            in IRadioImsIndication radioImsIndication);
}
