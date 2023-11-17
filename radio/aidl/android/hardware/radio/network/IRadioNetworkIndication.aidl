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

import android.hardware.radio.RadioIndicationType;
import android.hardware.radio.RadioTechnology;
import android.hardware.radio.network.BarringInfo;
import android.hardware.radio.network.CellIdentity;
import android.hardware.radio.network.CellInfo;
import android.hardware.radio.network.CellularIdentifierDisclosure;
import android.hardware.radio.network.EmergencyRegResult;
import android.hardware.radio.network.LinkCapacityEstimate;
import android.hardware.radio.network.NetworkScanResult;
import android.hardware.radio.network.PhoneRestrictedState;
import android.hardware.radio.network.PhysicalChannelConfig;
import android.hardware.radio.network.SecurityAlgorithmUpdate;
import android.hardware.radio.network.SignalStrength;
import android.hardware.radio.network.SuppSvcNotification;

/**
 * Interface declaring unsolicited radio indications for network APIs.
 * @hide
 */
@VintfStability
oneway interface IRadioNetworkIndication {
    /**
     * Indicate barring information for the user’s access category / access class and PLMN.
     *
     * <p>Provide information about the barring status of the cell for the user. The information
     * provided should describe all barring configurations that are applicable to the current user,
     * even if the user is not currently barred (due to conditional barring). This informs Android
     * of likely future (statistical) barring for specific services.
     *
     * <p>This indication should be sent whenever the cell’s barring config changes for the current
     * user, or if the user’s conditional barring status changes due to re-evaluation of the
     * barring conditions. Barring status will likely change when the device camps for service,
     * when PLMN selection is completed, when the device attempts to access a conditionally barred
     * service, and when the System Information including barring info for a camped cell is updated.
     *
     * @param type Type of radio indication
     * @param cellIdentity cellIdentity for the barring infos
     * @param barringInfos a vector of BarringInfos for all barring service types
     */
    void barringInfoChanged(in RadioIndicationType type, in CellIdentity cellIdentity,
            in BarringInfo[] barringInfos);

    /**
     * Indicates when PRL (preferred roaming list) changes.
     *
     * @param type Type of radio indication
     * @param version PRL version after PRL changes
     */
    void cdmaPrlChanged(in RadioIndicationType type, in int version);

    /**
     * Report all of the current cell information known to the radio.
     *
     * @param type Type of radio indication
     * @param records Current cell information
     */
    void cellInfoList(in RadioIndicationType type, in CellInfo[] records);

    /**
     * Indicates current link capacity estimate. This indication is sent whenever the reporting
     * criteria, as set by IRadioNetwork.setLinkCapacityReportingCriteria(), are met and the
     * indication is not suppressed by IRadioNetwork.setIndicationFilter().
     *
     * @param type Type of radio indication
     * @param lce LinkCapacityEstimate
     */
    void currentLinkCapacityEstimate(in RadioIndicationType type, in LinkCapacityEstimate lce);

    /**
     * Indicates physical channel configurations. An empty configs list shall be returned when the
     * radio is in idle mode (i.e. RRC idle).
     *
     * @param type Type of radio indication
     * @param configs Vector of PhysicalChannelConfigs
     */
    void currentPhysicalChannelConfigs(
            in RadioIndicationType type, in PhysicalChannelConfig[] configs);

    /**
     * Indicates current signal strength of the radio.
     *
     * @param type Type of radio indication
     * @param signalStrength SignalStrength information
     */
    void currentSignalStrength(in RadioIndicationType type, in SignalStrength signalStrength);

    /**
     * Indicates when IMS registration state has changed. To get IMS registration state and IMS SMS
     * format, callee needs to invoke getImsRegistrationState().
     *
     * @param type Type of radio indication
     */
    void imsNetworkStateChanged(in RadioIndicationType type);

    /**
     * Incremental network scan results.
     *
     * @param type Type of radio indication
     * @param result the result of the network scan
     */
    void networkScanResult(in RadioIndicationType type, in NetworkScanResult result);

    /**
     * Indicates when voice or data network state changed. Callee must invoke
     * IRadioNetwork.getVoiceRegistrationState(), IRadioNetwork.getDataRegistrationState(), and
     * IRadioNetwork.getOperator()
     *
     * @param type Type of radio indication
     */
    void networkStateChanged(in RadioIndicationType type);

    /**
     * Indicates when radio has received a NITZ time message.
     *
     * @param type Type of radio indication
     * @param nitzTime NITZ time string in the form "yy/mm/dd,hh:mm:ss(+/-)tz,dt"
     * @param receivedTimeMs time (in milliseconds since boot) at which RIL sent the NITZ time to
     *        the framework
     * @param ageMs time in milliseconds indicating how long NITZ was cached in RIL and modem.
     *        This must track true age and therefore must be calculated using clocks that
     *        include the time spend in sleep / low power states. If it can not be guaranteed,
     *        there must not be any caching done at the modem and should fill in 0 for ageMs
     */
    void nitzTimeReceived(
            in RadioIndicationType type, in String nitzTime, in long receivedTimeMs, in long ageMs);

    /**
     * Report that Registration or a Location/Routing/Tracking Area update has failed.
     *
     * <p>Indicate whenever a registration procedure, including a location, routing, or tracking
     * area update fails. This includes procedures that do not necessarily result in a change of
     * the modem's registration status. If the modem's registration status changes, that is
     * reflected in the onNetworkStateChanged() and subsequent get{Voice/Data}RegistrationState().
     *
     * @param cellIdentity the CellIdentity, which must include the globally unique identifier for
     *        the cell (for example, all components of the CGI or ECGI).
     * @param chosenPlmn a 5 or 6 digit alphanumeric PLMN (MCC|MNC) among those broadcast by the
     *        cell that was chosen for the failed registration attempt.
     * @param domain Domain::CS, Domain::PS, or both in case of a combined procedure.
     * @param causeCode the primary failure cause code of the procedure.
     *        For GSM/UMTS (MM), values are in TS 24.008 Sec 10.5.95
     *        For GSM/UMTS (GMM), values are in TS 24.008 Sec 10.5.147
     *        For LTE (EMM), cause codes are TS 24.301 Sec 9.9.3.9
     *        For NR (5GMM), cause codes are TS 24.501 Sec 9.11.3.2
     *        MAX_INT if this value is unused.
     * @param additionalCauseCode the cause code of any secondary/combined procedure if appropriate.
     *        For UMTS, if a combined attach succeeds for PS only, then the GMM cause code shall be
     *        included as an additionalCauseCode.
     *        For LTE (ESM), cause codes are in TS 24.301 9.9.4.4
     *        MAX_INT if this value is unused.
     */
    void registrationFailed(in RadioIndicationType type, in CellIdentity cellIdentity,
            in String chosenPlmn, in int domain, in int causeCode, in int additionalCauseCode);

    /**
     * Indicates a restricted state change (eg, for Domain Specific Access Control).
     * Radio must send this msg after radio off/on cycle no matter it is changed or not.
     *
     * @param type Type of radio indication
     * @param state Bitmask of restricted state as defined by PhoneRestrictedState
     */
    void restrictedStateChanged(in RadioIndicationType type, in PhoneRestrictedState state);

    /**
     * Reports supplementary service related notification from the network.
     *
     * @param type Type of radio indication
     * @param suppSvc SuppSvcNotification
     */
    void suppSvcNotify(in RadioIndicationType type, in SuppSvcNotification suppSvc);

    /**
     * Indicates that voice technology has changed. Responds with new rat.
     *
     * @param type Type of radio indication
     * @param rat Current new voice rat
     */
    void voiceRadioTechChanged(in RadioIndicationType type, in RadioTechnology rat);

    /**
     * Emergency Scan Results.
     *
     * @param type Type of radio indication
     * @param result the result of the Emergency Network Scan
     */
    void emergencyNetworkScanResult(in RadioIndicationType type, in EmergencyRegResult result);

    /**
     * Report a cellular identifier disclosure event. See
     * IRadioNetwork.setCellularIdnetifierTransparencyEnabled for more details.
     *
     * A non-exhaustive list of when this method should be called follows:
     *
     * - If a device attempts an IMSI attach to the network.
     * - If a device includes an IMSI in the IDENTITY_RESPONSE message on the NAS and a security
     * context has not yet been established.
     * - If a device includes an IMSI in a DETACH_REQUEST message sent on the NAS and the message is
     * sent before a security context has been established.
     * - If a device includes an IMSI in a TRACKING_AREA_UPDATE message sent on the NAS and the
     * message is sent before a security context has been established.
     * - If a device uses a 2G network to send a LOCATION_UPDATE_REQUEST message on the NAS that
     * includes an IMSI or IMEI.
     * - If a device uses a 2G network to send a AUTHENTICATION_AND_CIPHERING_RESPONSE message on
     * the NAS and the message includes an IMEISV.
     *
     * @param type Type of radio indication
     * @param disclosure A CellularIdentifierDisclosure as specified by
     *         IRadioNetwork.setCellularIdentifierTransparencyEnabled.
     *
     */
    void cellularIdentifierDisclosed(
            in RadioIndicationType type, in CellularIdentifierDisclosure disclosure);

    /*
     * Indicates that a new ciphering or integrity algorithm was used for a particular voice,
     * signaling, or data connection attempt for a given PLMN and/or access network. Due to
     * power concerns, once a connection type has been reported on, follow-up reports about that
     * connection type are only generated if there is any change to the previously reported
     * encryption or integrity. Thus the AP is only to be notified when there is new information.
     * List is reset upon rebooting thus info about initial connections is always passed to the
     * AP after a reboot. List is also reset if the SIM is changed or if there has been a change
     * in the access network.
     *
     * Note: a change only in cell ID should not trigger an update, as the design is intended to
     * be agnostic to dual connectivity ("secondary serving cells").
     *
     * @param type Type of radio indication
     * @param securityAlgorithmUpdate SecurityAlgorithmUpdate encapsulates details of security
     *         algorithm updates
     */
    void securityAlgorithmsUpdated(
            in RadioIndicationType type, in SecurityAlgorithmUpdate securityAlgorithmUpdate);
}
