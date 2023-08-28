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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.supplicant.NetworkRequestEapSimGsmAuthParams;
import android.hardware.wifi.supplicant.NetworkRequestEapSimUmtsAuthParams;
import android.hardware.wifi.supplicant.TransitionDisableIndication;

/**
 * Callback Interface exposed by the supplicant service
 * for each network (ISupplicantStaNetwork).
 *
 * Clients need to host an instance of this AIDL interface object and
 * pass a reference of the object to the supplicant via the
 * corresponding |ISupplicantStaNetwork.registerCallback| method.
 */
@VintfStability
oneway interface ISupplicantStaNetworkCallback {
    /**
     * Used to request EAP Identity for this particular network.
     *
     * The response for the request must be sent using the corresponding
     * |ISupplicantNetwork.sendNetworkEapIdentityResponse| call.
     */
    void onNetworkEapIdentityRequest();

    /**
     * Used to request EAP GSM SIM authentication for this particular network.
     *
     * The response for the request must be sent using the corresponding
     * |ISupplicantNetwork.sendNetworkEapSimGsmAuthResponse| call.
     *
     * @param params Params associated with the request.
     */
    void onNetworkEapSimGsmAuthRequest(in NetworkRequestEapSimGsmAuthParams params);

    /**
     * Used to request EAP UMTS SIM authentication for this particular network.
     *
     * The response for the request must be sent using the corresponding
     * |ISupplicantNetwork.sendNetworkEapSimUmtsAuthResponse| call.
     *
     * @param params Params associated with the request.
     */
    void onNetworkEapSimUmtsAuthRequest(in NetworkRequestEapSimUmtsAuthParams params);

    /**
     * Used to notify WPA3 transition disable.
     */
    void onTransitionDisable(in TransitionDisableIndication ind);

    /**
     * Used to notify EAP certificate event.
     *
     * On receiving a server certifidate from TLS handshake, send this certificate
     * to the framework for Trust On First Use.
     */
    void onServerCertificateAvailable(
            in int depth, in byte[] subject, in byte[] certHash, in byte[] certBlob);

    /**
     * Used to notify the AT_PERMANENT_ID_REQ denied event.
     *
     * In strict conservative mode, AT_PERMANENT_ID_REQ is denied from eap_peer side.
     */
    void onPermanentIdReqDenied();
}
