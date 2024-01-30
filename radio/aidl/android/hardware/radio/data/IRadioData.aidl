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

import android.hardware.radio.AccessNetwork;
import android.hardware.radio.data.DataProfileInfo;
import android.hardware.radio.data.DataRequestReason;
import android.hardware.radio.data.DataThrottlingAction;
import android.hardware.radio.data.IRadioDataIndication;
import android.hardware.radio.data.IRadioDataResponse;
import android.hardware.radio.data.KeepaliveRequest;
import android.hardware.radio.data.LinkAddress;
import android.hardware.radio.data.SliceInfo;
import android.hardware.radio.data.TrafficDescriptor;

/**
 * This interface is used by telephony and telecom to talk to cellular radio for data APIs.
 * All the functions have minimum one parameter:
 * serial: which corresponds to serial no. of request. Serial numbers must only be memorized for the
 * duration of a method call. If clients provide colliding serials (including passing the same
 * serial to different methods), multiple responses (one for each method call) must still be served.
 * setResponseFunctions must work with IRadioDataResponse and IRadioDataIndication.
 * @hide
 */
@VintfStability
oneway interface IRadioData {
    /**
     * Reserves an unallocated pdu session id from the pool of ids. The allocated id is returned
     * in the response. When the id is no longer needed, call releasePduSessionId to return it to
     * the pool.
     *
     * Reference: 3GPP TS 24.007 section 11.2.3.1b
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioDataResponse.allocatePduSessionIdResponse()
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void allocatePduSessionId(in int serial);

    /**
     * Indicates that a handover was cancelled after a call to IRadioData::startHandover.
     * Since the handover was unsuccessful, the modem retains ownership over any of the resources
     * being transferred and is still responsible for releasing them.
     *
     * @param serial Serial number of request.
     * @param id callId The identifier of the data call which is provided in SetupDataCallResult
     *
     * Response function is IRadioDataResponse.cancelHandoverResponse()
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void cancelHandover(in int serial, in int callId);

    /**
     * Deactivate packet data connection and remove from the data call list. An
     * unsolDataCallListChanged() must be sent when data connection is deactivated.
     * Any return value other than RadioError::NONE will remove the network from the list.
     *
     * @param serial Serial number of request.
     * @param cid Data call id.
     * @param reason The request reason. Must be normal, handover, or shutdown.
     *
     * Response function is IRadioDataResponse.deactivateDataCallResponse()
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void deactivateDataCall(in int serial, in int cid, in DataRequestReason reason);

    /**
     * Returns the data call list. An entry is added when a setupDataCall() is issued and removed
     * on a deactivateDataCall(). The list is emptied when the vendor HAL crashes.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioDataResponse.getDataCallListResponse()
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void getDataCallList(in int serial);

    /**
     * Request to get the current slicing configuration including URSP rules and NSSAIs
     * (configured, allowed and rejected). URSP stands for UE route selection policy and is defined
     * in 3GPP TS 24.526 Section 4.2. An NSSAI is a collection of network slices. Each network slice
     * is identified by an S-NSSAI and is represented by the struct SliceInfo. NSSAI and S-NSSAI
     * are defined in 3GPP TS 24.501.
     *
     * @param serial Serial number of request.
     *
     * Response function is IRadioDataResponse.getSlicingConfigResponse()
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void getSlicingConfig(in int serial);

    /**
     * Releases a pdu session id that was previously allocated using allocatePduSessionId.
     * Reference: 3GPP TS 24.007 section 11.2.3.1b
     *
     * @param serial Serial number of request.
     * @param id Pdu session id to release.
     *
     * Response function is IRadioDataResponse.releasePduSessionIdResponse()
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void releasePduSessionId(in int serial, in int id);

    /**
     * When response type received from a radio indication or radio response is
     * RadioIndicationType:UNSOLICITED_ACK_EXP or RadioResponseType:SOLICITED_ACK_EXP respectively,
     * acknowledge the receipt of those messages by sending responseAcknowledgement().
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void responseAcknowledgement();

    /**
     * Tells the modem whether data calls are allowed or not
     *
     * @param serial Serial number of request.
     * @param allow true to allow data calls, false to disallow data calls
     *
     * Response function is IRadioDataResponse.setDataAllowedResponse()
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void setDataAllowed(in int serial, in boolean allow);

    /**
     * Send data profiles of the current carrier to the modem.
     *
     * @param serial Serial number of request.
     * @param profiles Array of DataProfileInfo to set.
     *
     * Response function is IRadioDataResponse.setDataProfileResponse()
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void setDataProfile(in int serial, in DataProfileInfo[] profiles);

    /**
     * Control data throttling at modem.
     * - DataThrottlingAction:NO_DATA_THROTTLING should clear any existing data throttling within
     *   the requested completion window.
     * - DataThrottlingAction:THROTTLE_SECONDARY_CARRIER: Remove any existing throttling on anchor
     *   carrier and achieve maximum data throttling on secondary carrier within the requested
     *   completion window.
     * - DataThrottlingAction:THROTTLE_ANCHOR_CARRIER: disable secondary carrier and achieve maximum
     *   data throttling on anchor carrier by requested completion window.
     * - DataThrottlingAction:HOLD: Immediately hold on to current level of throttling.
     *
     * @param serial Serial number of request.
     * @param dataThrottlingAction DataThrottlingAction as defined in types.hal
     * @param completionDurationMillis window, in milliseconds, in which the requested throttling
     *        action has to be achieved. This must be 0 when dataThrottlingAction is
     *        DataThrottlingAction:HOLD.
     *
     * Response function is IRadioDataResponse.setDataThrottlingResponse()
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void setDataThrottling(in int serial, in DataThrottlingAction dataThrottlingAction,
            in long completionDurationMillis);

    /**
     * Set an APN to initial attach network or clear the existing initial attach APN.
     *
     * @param serial Serial number of request.
     * @param dataProfileInfo Data profile containing APN settings or null to clear the existing
     *        initial attach APN.
     *
     * Response function is IRadioDataResponse.setInitialAttachApnResponse()
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void setInitialAttachApn(in int serial, in @nullable DataProfileInfo dataProfileInfo);

    /**
     * Set response functions for data radio requests and indications.
     *
     * @param radioDataResponse Object containing response functions
     * @param radioDataIndication Object containing radio indications
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void setResponseFunctions(
            in IRadioDataResponse radioDataResponse, in IRadioDataIndication radioDataIndication);

    /**
     * Setup a packet data connection. If DataCallResponse.status returns DataCallFailCause:NONE,
     * the data connection must be added to data calls and a unsolDataCallListChanged() must be
     * sent. The call remains until removed by subsequent unsolDataCallIstChanged(). It may be lost
     * due to many factors, including deactivateDataCall() being issued, the radio powered off,
     * reception lost or even transient factors like congestion. This data call list is returned by
     * getDataCallList() and dataCallListChanged().
     * The Radio is expected to:
     * - Create one data call context.
     * - Create and configure a dedicated interface for the context.
     * - The interface must be point to point.
     * - The interface is configured with one or more addresses and is capable of sending and
     *   receiving packets. The format is IP address with optional "/" prefix length (The format is
     *   defined in RFC-4291 section 2.3). For example, "192.0.1.3", "192.0.1.11/16", or
     *   "2001:db8::1/64". Typically one IPv4 or one IPv6 or one of each. If the prefix length is
     *   absent, then the addresses are assumed to be point to point with IPv4 with prefix length 32
     *   or IPv6 with prefix length 128.
     * - Must not modify routing configuration related to this interface; routing management is
     *   exclusively within the purview of the Android OS.
     * - Support simultaneous data call context, with limits defined in the specifications. For LTE,
     *   the max number of data calls is equal to the max number of EPS bearers that can be active.
     *
     * @param serial Serial number of request.
     * @param accessNetwork The access network to setup the data call. If the data connection cannot
     *        be established on the specified access network then this should respond with an error.
     * @param dataProfileInfo Data profile info.
     * @param roamingAllowed Indicates whether or not data roaming is allowed by the user.
     * @param reason The request reason. Must be DataRequestReason:NORMAL or
     *        DataRequestReason:HANDOVER.
     * @param addresses If the reason is DataRequestReason:HANDOVER, this indicates the list of link
     *        addresses of the existing data connection. This parameter must be ignored unless
     *        reason is DataRequestReason:HANDOVER.
     * @param dnses If the reason is DataRequestReason:HANDOVER, this indicates the list of DNS
     *        addresses of the existing data connection. The format is defined in RFC-4291 section
     *        2.2. For example, "192.0.1.3" or "2001:db8::1". This parameter must be ignored unless
     *        reason is DataRequestReason:HANDOVER.
     * @param pduSessionId The pdu session id to be used for this data call. A value of 0 means no
     *        pdu session id was attached to this call. Reference: 3GPP TS 24.007 section 11.2.3.1b
     * @param sliceInfo SliceInfo to be used for the data connection when a handover occurs from
     *        EPDG to 5G. It is valid only when accessNetwork is AccessNetwork:NGRAN. If the slice
     *        passed from EPDG is rejected, then the data failure cause must be
     *        DataCallFailCause:SLICE_REJECTED.
     * @param matchAllRuleAllowed bool to indicate if using default match-all URSP rule for this
     *        request is allowed. If false, this request must not use the match-all URSP rule and if
     *        a non-match-all rule is not found (or if URSP rules are not available) it should
     *        return failure with cause DataCallFailCause:MATCH_ALL_RULE_NOT_ALLOWED. This is needed
     *        as some requests need to have a hard failure if the intention cannot be met, for
     *        example, a zero-rating slice.
     *
     * Response function is IRadioDataResponse.setupDataCallResponse()
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void setupDataCall(in int serial, in AccessNetwork accessNetwork,
            in DataProfileInfo dataProfileInfo, in boolean roamingAllowed,
            in DataRequestReason reason, in LinkAddress[] addresses, in String[] dnses,
            in int pduSessionId, in @nullable SliceInfo sliceInfo,
            in boolean matchAllRuleAllowed);

    /**
     * Indicates that a handover to the IWLAN transport has begun. Any resources being transferred
     * to the IWLAN transport cannot be released while a handover is underway. For example, if a
     * pdu session id needs to be transferred to IWLAN, then the modem should not release the id
     * while the handover is in progress. If a handover was unsuccessful, then the framework calls
     * IRadio::cancelHandover. The modem retains ownership over any of the resources being
     * transferred to IWLAN. If a handover was successful, the framework calls
     * IRadio::deactivateDataCall with reason HANDOVER. The IWLAN transport now owns the transferred
     * resources and is responsible for releasing them.
     *
     * @param serial Serial number of request.
     * @param id callId The identifier of the data call which is provided in SetupDataCallResult
     *
     * Response function is IRadioDataResponse.startHandoverResponse()
     *
     * This is available when android.hardware.telephony.ims is defined.
     */
    void startHandover(in int serial, in int callId);

    /**
     * Start a Keepalive session (for IPsec)
     *
     * @param serial Serial number of request.
     * @param keepalive A request structure containing all necessary info to describe a keepalive
     *
     * Response function is IRadioDataResponse.startKeepaliveResponse()
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void startKeepalive(in int serial, in KeepaliveRequest keepalive);

    /**
     * Stop an ongoing Keepalive session (for IPsec)
     *
     * @param serial Serial number of request.
     * @param sessionHandle The handle that was provided by
     *        IRadioDataResponse.startKeepaliveResponse
     *
     * Response function is IRadioDataResponse.stopKeepaliveResponse()
     *
     * This is available when android.hardware.telephony.data is defined.
     */
    void stopKeepalive(in int serial, in int sessionHandle);
}
