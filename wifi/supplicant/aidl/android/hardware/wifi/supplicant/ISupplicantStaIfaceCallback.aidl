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

import android.hardware.wifi.supplicant.AnqpData;
import android.hardware.wifi.supplicant.AssociationRejectionData;
import android.hardware.wifi.supplicant.BssTmData;
import android.hardware.wifi.supplicant.BssidChangeReason;
import android.hardware.wifi.supplicant.DppAkm;
import android.hardware.wifi.supplicant.DppEventType;
import android.hardware.wifi.supplicant.DppFailureCode;
import android.hardware.wifi.supplicant.DppProgressCode;
import android.hardware.wifi.supplicant.Hs20AnqpData;
import android.hardware.wifi.supplicant.OsuMethod;
import android.hardware.wifi.supplicant.StaIfaceCallbackState;
import android.hardware.wifi.supplicant.StaIfaceReasonCode;
import android.hardware.wifi.supplicant.WpsConfigError;
import android.hardware.wifi.supplicant.WpsErrorIndication;

/**
 * Callback Interface exposed by the supplicant service
 * for each station mode interface (ISupplicantStaIface).
 *
 * Clients need to host an instance of this AIDL interface object and
 * pass a reference of the object to the supplicant via the
 * corresponding |ISupplicantStaIface.registerCallback| method.
 */
@VintfStability
interface ISupplicantStaIfaceCallback {
    /**
     * Used to indicate the result of ANQP (either for IEEE 802.11u Interworking
     * or Hotspot 2.0) query.
     *
     * @param bssid BSSID of the access point.
     * @param data ANQP data fetched from the access point.
     *        All the fields in this struct must be empty if the query failed.
     * @param hs20Data ANQP data fetched from the Hotspot 2.0 access point.
     *        All the fields in this struct must be empty if the query failed.
     */
    oneway void onAnqpQueryDone(in byte[] bssid, in AnqpData data, in Hs20AnqpData hs20Data);

    /**
     * Used to indicate an association rejection received from the AP
     * to which the connection is being attempted.
     *
     * @param assocRejectData Association Rejection related information.
     */
    oneway void onAssociationRejected(in AssociationRejectionData assocRejectData);

    /**
     * Used to indicate the timeout of authentication to an AP.
     *
     * @param bssid BSSID of the corresponding AP.
     */
    oneway void onAuthenticationTimeout(in byte[] bssid);

    /**
     * Indicates BTM request frame handling status.
     *
     * @param tmData Data retrieved from received BSS transition management
     * request frame.
     */
    oneway void onBssTmHandlingDone(in BssTmData tmData);

    /**
     * Used to indicate the change of active bssid.
     * This is useful to figure out when the driver/firmware roams to a bssid
     * on its own.
     *
     * @param reason Reason why the bssid changed.
     * @param bssid BSSID of the corresponding AP.
     */
    oneway void onBssidChanged(in BssidChangeReason reason, in byte[] bssid);

    /**
     * Used to indicate the disconnection from the currently connected
     * network on this iface.
     *
     * @param bssid BSSID of the AP from which we disconnected.
     * @param locallyGenerated If the disconnect was triggered by
     *        wpa_supplicant.
     * @param reasonCode 802.11 code to indicate the disconnect reason
     *        from access point. Refer to section 8.4.1.7 of IEEE802.11 spec.
     */
    oneway void onDisconnected(
            in byte[] bssid, in boolean locallyGenerated, in StaIfaceReasonCode reasonCode);

    /**
     * Indicates a DPP failure event.
     *
     * ssid: A string indicating the SSID for the AP that the Enrollee attempted to connect.
     * channelList: A string containing a list of operating channels and operating classes
     *     indicating the channels that the Enrollee scanned in attempting to discover the AP.
     *     The list conforms to the following ABNF syntax:
     *         channel-list2 = class-and-channels *(“,” class-and-channels)
     *         class-and-channels = class “/” channel *(“,” channel)
     *         class = 1*3DIGIT
     *         channel = 1*3DIGIT
     * bandList: A list of band parameters that are supported by the Enrollee expressed as the
     *     Operating Class.
     */
    oneway void onDppFailure(
            in DppFailureCode code, in String ssid, in String channelList, in char[] bandList);

    /**
     * Indicates a DPP progress event.
     */
    oneway void onDppProgress(in DppProgressCode code);

    /**
     * Indicates a DPP success event.
     */
    oneway void onDppSuccess(in DppEventType event);

    /**
     * Indicates DPP configuration received success event (Enrolee mode).
     */
    oneway void onDppSuccessConfigReceived(
            in byte[] ssid, in String password, in byte[] psk, in DppAkm securityAkm);

    /**
     * Indicates DPP configuration sent success event (Configurator mode).
     */
    oneway void onDppSuccessConfigSent();

    /**
     * Indicates an EAP authentication failure.
     * @param errorCode Error code for EAP authentication failure.
     *        Either standard error code (enum EapErrorCode) or
     *        private error code defined by network provider.
     */
    oneway void onEapFailure(in int errorCode);

    /**
     * Used to indicate that the external radio work can start now.
     *
     * @param id Identifier generated for the radio work request.
     */
    oneway void onExtRadioWorkStart(in int id);

    /**
     * Used to indicate that the external radio work request has timed out.
     *
     * @param id Identifier generated for the radio work request.
     */
    oneway void onExtRadioWorkTimeout(in int id);

    /**
     * Used to indicate a Hotspot 2.0 imminent deauth notice.
     *
     * @param bssid BSSID of the access point.
     * @param reasonCode Code to indicate the deauth reason.
     *        Refer to section 3.2.1.2 of the Hotspot 2.0 spec.
     * @param reAuthDelayInSec Delay before reauthenticating.
     * @param url URL of the server.
     */
    oneway void onHs20DeauthImminentNotice(
            in byte[] bssid, in int reasonCode, in int reAuthDelayInSec, in String url);

    /**
     * Used to indicate the result of Hotspot 2.0 Icon query.
     *
     * @param bssid BSSID of the access point.
     * @param fileName Name of the file that was requested.
     * @param data Icon data fetched from the access point.
     *        Must be empty if the query failed.
     */
    oneway void onHs20IconQueryDone(in byte[] bssid, in String fileName, in byte[] data);

    /**
     * Used to indicate a Hotspot 2.0 subscription remediation event.
     *
     * @param bssid BSSID of the access point.
     * @param osuMethod OSU method.
     * @param url URL of the server.
     */
    oneway void onHs20SubscriptionRemediation(
            in byte[] bssid, in OsuMethod osuMethod, in String url);

    /**
     * Used to indicate a Hotspot 2.0 terms and conditions acceptance is requested from the user
     * before allowing the device to get internet access.
     *
     * @param bssid BSSID of the access point.
     * @param url URL of the T&C server.
     */
    oneway void onHs20TermsAndConditionsAcceptanceRequestedNotification(
            in byte[] bssid, in String url);

    /**
     * Used to indicate that a new network has been added.
     *
     * @param id Network ID allocated to the corresponding network.
     */
    oneway void onNetworkAdded(in int id);

    /**
     * Used to indicate that the supplicant failed to find a network in scan result
     * which matches with the network capabilities requested by upper layer
     * for connection.
     *
     * @param ssid network name supplicant tried to connect.
     */
    oneway void onNetworkNotFound(in byte[] ssid);

    /**
     * Used to indicate that a network has been removed.
     *
     * @param id Network ID allocated to the corresponding network.
     */
    oneway void onNetworkRemoved(in int id);

    /**
     * Indicates pairwise master key (PMK) cache added event.
     *
     * @param expirationTimeInSec expiration time in seconds
     * @param serializedEntry is serialized PMK cache entry, the content is
     *              opaque for the framework and depends on the native implementation.
     */
    oneway void onPmkCacheAdded(in long expirationTimeInSec, in byte[] serializedEntry);

    /**
     * Used to indicate a state change event on this particular iface. If this
     * event is triggered by a particular network, the |SupplicantNetworkId|,
     * |ssid|, |bssid| parameters must indicate the parameters of the network/AP
     * which caused this state transition.
     *
     * @param newState New State of the interface. This must be one of the |State|
     *        values above.
     * @param bssid BSSID of the corresponding AP which caused this state
     *        change event. This must be zero'ed if this event is not
     *        specific to a particular network.
     * @param id ID of the corresponding network which caused this
     *        state change event. This must be invalid (UINT32_MAX) if this
     *        event is not specific to a particular network.
     * @param ssid SSID of the corresponding network which caused this state
     *        change event. This must be empty if this event is not specific
     *        to a particular network.
     * @param filsHlpSent If FILS HLP IEs were included in this association.
     */
    oneway void onStateChanged(in StaIfaceCallbackState newState, in byte[] bssid, in int id,
            in byte[] ssid, in boolean filsHlpSent);

    /**
     * Used to indicate the failure of a WPS connection attempt.
     *
     * @param bssid BSSID of the AP to which we initiated WPS
     *        connection.
     * @param configError Configuration error code.
     * @param errorInd Error indication code.
     */
    oneway void onWpsEventFail(
            in byte[] bssid, in WpsConfigError configError, in WpsErrorIndication errorInd);

    /**
     * Used to indicate the overlap of a WPS PBC connection attempt.
     */
    oneway void onWpsEventPbcOverlap();

    /**
     * Used to indicate the success of a WPS connection attempt.
     */
    oneway void onWpsEventSuccess();
}
