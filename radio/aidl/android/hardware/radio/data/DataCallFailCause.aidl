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

/** @hide */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum DataCallFailCause {
    /**
     * An integer cause code defined in TS 24.008 section 6.1.3.1.3 or TS 24.301 Release 8+ Annex B.
     * If the implementation does not have access to the exact cause codes, then it must return one
     * of the following values, as the UI layer needs to distinguish these cases for error
     * notification and potential retries.
     */
    NONE = 0,
    /**
     * No retry
     */
    OPERATOR_BARRED = 0x08,
    /**
     * PDP_FAIL_LLC_SNDCP = 0x19
     */
    NAS_SIGNALLING = 0x0E,
    INSUFFICIENT_RESOURCES = 0x1A,
    /**
     * No retry
     */
    MISSING_UNKNOWN_APN = 0x1B,
    /**
     * No retry
     */
    UNKNOWN_PDP_ADDRESS_TYPE = 0x1C,
    /**
     * No retry
     */
    USER_AUTHENTICATION = 0x1D,
    /**
     * No retry
     */
    ACTIVATION_REJECT_GGSN = 0x1E,
    ACTIVATION_REJECT_UNSPECIFIED = 0x1F,
    /**
     * No retry
     */
    SERVICE_OPTION_NOT_SUPPORTED = 0x20,
    /**
     * No retry
     */
    SERVICE_OPTION_NOT_SUBSCRIBED = 0x21,
    SERVICE_OPTION_OUT_OF_ORDER = 0x22,
    /**
     * No retry
     */
    NSAPI_IN_USE = 0x23,
    /**
     * Possibly restart radio, based on framework config
     */
    REGULAR_DEACTIVATION = 0x24,
    QOS_NOT_ACCEPTED = 0x25,
    NETWORK_FAILURE = 0x26,
    UMTS_REACTIVATION_REQ = 0x27,
    FEATURE_NOT_SUPP = 0x28,
    TFT_SEMANTIC_ERROR = 0x29,
    TFT_SYTAX_ERROR = 0x2A,
    UNKNOWN_PDP_CONTEXT = 0x2B,
    FILTER_SEMANTIC_ERROR = 0x2C,
    FILTER_SYTAX_ERROR = 0x2D,
    PDP_WITHOUT_ACTIVE_TFT = 0x2E,
    /**
     * No retry
     */
    ONLY_IPV4_ALLOWED = 0x32,
    /**
     * No retry
     */
    ONLY_IPV6_ALLOWED = 0x33,
    ONLY_SINGLE_BEARER_ALLOWED = 0x34,
    ESM_INFO_NOT_RECEIVED = 0x35,
    PDN_CONN_DOES_NOT_EXIST = 0x36,
    MULTI_CONN_TO_SAME_PDN_NOT_ALLOWED = 0x37,
    MAX_ACTIVE_PDP_CONTEXT_REACHED = 0x41,
    UNSUPPORTED_APN_IN_CURRENT_PLMN = 0x42,
    INVALID_TRANSACTION_ID = 0x51,
    MESSAGE_INCORRECT_SEMANTIC = 0x5F,
    INVALID_MANDATORY_INFO = 0x60,
    MESSAGE_TYPE_UNSUPPORTED = 0x61,
    MSG_TYPE_NONCOMPATIBLE_STATE = 0x62,
    UNKNOWN_INFO_ELEMENT = 0x63,
    CONDITIONAL_IE_ERROR = 0x64,
    MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE = 0x65,
    /**
     * No retry
     */
    PROTOCOL_ERRORS = 0x6F,
    APN_TYPE_CONFLICT = 0x70,
    INVALID_PCSCF_ADDR = 0x71,
    INTERNAL_CALL_PREEMPT_BY_HIGH_PRIO_APN = 0x72,
    EMM_ACCESS_BARRED = 0x73,
    EMERGENCY_IFACE_ONLY = 0x74,
    IFACE_MISMATCH = 0x75,
    COMPANION_IFACE_IN_USE = 0x76,
    IP_ADDRESS_MISMATCH = 0x77,
    IFACE_AND_POL_FAMILY_MISMATCH = 0x78,
    EMM_ACCESS_BARRED_INFINITE_RETRY = 0x79,
    AUTH_FAILURE_ON_EMERGENCY_CALL = 0x7A,
    OEM_DCFAILCAUSE_1 = 0x1001,
    OEM_DCFAILCAUSE_2 = 0x1002,
    OEM_DCFAILCAUSE_3 = 0x1003,
    OEM_DCFAILCAUSE_4 = 0x1004,
    OEM_DCFAILCAUSE_5 = 0x1005,
    OEM_DCFAILCAUSE_6 = 0x1006,
    OEM_DCFAILCAUSE_7 = 0x1007,
    OEM_DCFAILCAUSE_8 = 0x1008,
    OEM_DCFAILCAUSE_9 = 0x1009,
    OEM_DCFAILCAUSE_10 = 0x100A,
    OEM_DCFAILCAUSE_11 = 0x100B,
    OEM_DCFAILCAUSE_12 = 0x100C,
    OEM_DCFAILCAUSE_13 = 0x100D,
    OEM_DCFAILCAUSE_14 = 0x100E,
    OEM_DCFAILCAUSE_15 = 0x100F,
    /**
     * Not mentioned in the specification
     */
    VOICE_REGISTRATION_FAIL = -1,
    /**
     * Not mentioned in the specification
     */
    DATA_REGISTRATION_FAIL = -2,
    /**
     * Network/modem disonnect
     */
    SIGNAL_LOST = -3,
    /**
     * Preferred technology has changed, must retry with parameters appropriate for new technology
     */
    PREF_RADIO_TECH_CHANGED = -4,
    /**
     * Data call was disconnected because radio was resetting, powered off - no retry
     */
    RADIO_POWER_OFF = -5,
    /**
     * Data call was disconnected by modem because tethered mode was up on same APN/data profile
     * No retry until tethered call is off
     */
    TETHERED_CALL_ACTIVE = -6,
    ERROR_UNSPECIFIED = 0xffff,
    /**
     * Network cannot provide the requested service and PDP context is deactivated because of LLC
     * or SNDCP failure.
     */
    LLC_SNDCP = 0x19,
    /**
     * UE requested to modify QoS parameters or the bearer control mode, which is not compatible
     * with the selected bearer control mode.
     */
    ACTIVATION_REJECTED_BCM_VIOLATION = 0x30,
    /**
     * Network has already initiated the activation, modification, or deactivation of bearer
     * resources that was requested by the UE.
     */
    COLLISION_WITH_NETWORK_INITIATED_REQUEST = 0x38,
    /**
     * Network supports IPv4v6 PDP type only. Non-IP type is not allowed. In LTE mode of operation,
     * this is a PDN throttling cause code, meaning the UE may throttle further requests to the
     * same APN.
     */
    ONLY_IPV4V6_ALLOWED = 0x39,
    /**
     * Network supports non-IP PDP type only. IPv4, IPv6 and IPv4v6 is not allowed. In LTE mode of
     * operation, this is a PDN throttling cause code, meaning the UE can throttle further requests
     * to the same APN.
     */
    ONLY_NON_IP_ALLOWED = 0x3A,
    /**
     * QCI (QoS Class Identifier) indicated in the UE request cannot be supported.
     */
    UNSUPPORTED_QCI_VALUE = 0x3B,
    /**
     * Procedure requested by the UE was rejected because the bearer handling is not supported.
     */
    BEARER_HANDLING_NOT_SUPPORTED = 0x3C,
    /**
     * Not receiving a DNS address that was mandatory.
     */
    INVALID_DNS_ADDR = 0x7B,
    /**
     * Not receiving either a PCSCF or a DNS address, one of them being mandatory.
     */
    INVALID_PCSCF_OR_DNS_ADDRESS = 0x7C,
    /**
     * Emergency call bring up on a different ePDG.
     */
    CALL_PREEMPT_BY_EMERGENCY_APN = 0x7F,
    /**
     * UE performs a detach or disconnect PDN action based on TE requirements.
     */
    UE_INITIATED_DETACH_OR_DISCONNECT = 0x80,
    /**
     * Reason unspecified for foreign agent rejected MIP (Mobile IP) registration.
     */
    MIP_FA_REASON_UNSPECIFIED = 0x7D0,
    /**
     * Foreign agent administratively prohibited MIP (Mobile IP) registration.
     */
    MIP_FA_ADMIN_PROHIBITED = 0x7D1,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of insufficient resources.
     */
    MIP_FA_INSUFFICIENT_RESOURCES = 0x7D2,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of MN-AAA authenticator was
     * wrong.
     */
    MIP_FA_MOBILE_NODE_AUTHENTICATION_FAILURE = 0x7D3,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of home agent authentication
     * failure.
     */
    MIP_FA_HOME_AGENT_AUTHENTICATION_FAILURE = 0x7D4,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of requested lifetime was too
     * long.
     */
    MIP_FA_REQUESTED_LIFETIME_TOO_LONG = 0x7D5,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of malformed request.
     */
    MIP_FA_MALFORMED_REQUEST = 0x7D6,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of malformed reply.
     */
    MIP_FA_MALFORMED_REPLY = 0x7D7,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of requested encapsulation was
     * unavailable.
     */
    MIP_FA_ENCAPSULATION_UNAVAILABLE = 0x7D8,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration of VJ Header Compression was unavailable.
     */
    MIP_FA_VJ_HEADER_COMPRESSION_UNAVAILABLE = 0x7D9,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of reverse tunnel was
     * unavailable.
     */
    MIP_FA_REVERSE_TUNNEL_UNAVAILABLE = 0x7DA,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of reverse tunnel was mandatory
     * but not requested by device.
     */
    MIP_FA_REVERSE_TUNNEL_IS_MANDATORY = 0x7DB,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of delivery style was not
     * supported.
     */
    MIP_FA_DELIVERY_STYLE_NOT_SUPPORTED = 0x7DC,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of missing NAI (Network Access
     * Identifier).
     */
    MIP_FA_MISSING_NAI = 0x7DD,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of missing Home Agent.
     */
    MIP_FA_MISSING_HOME_AGENT = 0x7DE,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of missing Home Address.
     */
    MIP_FA_MISSING_HOME_ADDRESS = 0x7DF,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of unknown challenge.
     */
    MIP_FA_UNKNOWN_CHALLENGE = 0x7E0,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of missing challenge.
     */
    MIP_FA_MISSING_CHALLENGE = 0x7E1,
    /**
     * Foreign agent rejected MIP (Mobile IP) registration because of stale challenge.
     */
    MIP_FA_STALE_CHALLENGE = 0x7E2,
    /**
     * Reason unspecified for home agent rejected MIP (Mobile IP) registration.
     */
    MIP_HA_REASON_UNSPECIFIED = 0x7E3,
    /**
     * Home agent administratively prohibited MIP (Mobile IP) registration.
     */
    MIP_HA_ADMIN_PROHIBITED = 0x7E4,
    /**
     * Home agent rejected MIP (Mobile IP) registration because of insufficient resources.
     */
    MIP_HA_INSUFFICIENT_RESOURCES = 0x7E5,
    /**
     * Home agent rejected MIP (Mobile IP) registration because of MN-HA authenticator was wrong.
     */
    MIP_HA_MOBILE_NODE_AUTHENTICATION_FAILURE = 0x7E6,
    /**
     * Home agent rejected MIP (Mobile IP) registration because of foreign agent authentication
     * failure.
     */
    MIP_HA_FOREIGN_AGENT_AUTHENTICATION_FAILURE = 0x7E7,
    /**
     * Home agent rejected MIP (Mobile IP) registration because of registration id mismatch.
     */
    MIP_HA_REGISTRATION_ID_MISMATCH = 0x7E8,
    /**
     * Home agent rejected MIP (Mobile IP) registration because of malformed request.
     */
    MIP_HA_MALFORMED_REQUEST = 0x7E9,
    /**
     * Home agent rejected MIP (Mobile IP) registration because of unknown home agent address.
     */
    MIP_HA_UNKNOWN_HOME_AGENT_ADDRESS = 0x7EA,
    /**
     * Home agent rejected MIP (Mobile IP) registration because of reverse tunnel was unavailable.
     */
    MIP_HA_REVERSE_TUNNEL_UNAVAILABLE = 0x7EB,
    /**
     * Home agent rejected MIP (Mobile IP) registration because of reverse tunnel is mandatory but
     * not requested by device.
     */
    MIP_HA_REVERSE_TUNNEL_IS_MANDATORY = 0x7EC,
    /**
     * Home agent rejected MIP (Mobile IP) registration because of encapsulation unavailable.
     */
    MIP_HA_ENCAPSULATION_UNAVAILABLE = 0x7ED,
    /**
     * Tearing down is in progress.
     */
    CLOSE_IN_PROGRESS = 0x7EE,
    /**
     * Brought down by the network.
     */
    NETWORK_INITIATED_TERMINATION = 0x7EF,
    /**
     * Another application in modem preempts the data call.
     */
    MODEM_APP_PREEMPTED = 0x7F0,
    /**
     * IPV4 PDN is in throttled state due to network providing only IPV6 address during the previous
     * VSNCP bringup (subs_limited_to_v6).
     */
    PDN_IPV4_CALL_DISALLOWED = 0x7F1,
    /**
     * IPV4 PDN is in throttled state due to previous VSNCP bringup failure(s).
     */
    PDN_IPV4_CALL_THROTTLED = 0x7F2,
    /**
     * IPV6 PDN is in throttled state due to network providing only IPV4 address during the previous
     * VSNCP bringup (subs_limited_to_v4).
     */
    PDN_IPV6_CALL_DISALLOWED = 0x7F3,
    /**
     * IPV6 PDN is in throttled state due to previous VSNCP bringup failure(s).
     */
    PDN_IPV6_CALL_THROTTLED = 0x7F4,
    /**
     * Modem restart.
     */
    MODEM_RESTART = 0x7F5,
    /**
     * PDP PPP calls are not supported.
     */
    PDP_PPP_NOT_SUPPORTED = 0x7F6,
    /**
     * RAT on which the data call is attempted/connected is no longer the preferred RAT.
     */
    UNPREFERRED_RAT = 0x7F7,
    /**
     * Physical link is in the process of cleanup.
     */
    PHYSICAL_LINK_CLOSE_IN_PROGRESS = 0x7F8,
    /**
     * Interface bring up is attempted for an APN that is yet to be handed over to target RAT.
     */
    APN_PENDING_HANDOVER = 0x7F9,
    /**
     * APN bearer type in the profile does not match preferred network mode.
     */
    PROFILE_BEARER_INCOMPATIBLE = 0x7FA,
    /**
     * Card was refreshed or removed.
     */
    SIM_CARD_CHANGED = 0x7FB,
    /**
     * Device is going into lower power mode or powering down.
     */
    LOW_POWER_MODE_OR_POWERING_DOWN = 0x7FC,
    /**
     * APN has been disabled.
     */
    APN_DISABLED = 0x7FD,
    /**
     * Maximum PPP inactivity timer expired.
     */
    MAX_PPP_INACTIVITY_TIMER_EXPIRED = 0x7FE,
    /**
     * IPv6 address transfer failed.
     */
    IPV6_ADDRESS_TRANSFER_FAILED = 0x7FF,
    /**
     * Target RAT swap failed.
     */
    TRAT_SWAP_FAILED = 0x800,
    /**
     * Device falls back from eHRPD to HRPD.
     */
    EHRPD_TO_HRPD_FALLBACK = 0x801,
    /**
     * UE is in MIP-only configuration but the MIP configuration fails on call bring up due to
     * incorrect provisioning.
     */
    MIP_CONFIG_FAILURE = 0x802,
    /**
     * PDN inactivity timer expired due to no data transmission in a configurable duration of time.
     */
    PDN_INACTIVITY_TIMER_EXPIRED = 0x803,
    /**
     * IPv4 data call bring up is rejected because the UE already maintains the allotted maximum
     * number of IPv4 data connections.
     */
    MAX_IPV4_CONNECTIONS = 0x804,
    /**
     * IPv6 data call bring up is rejected because the UE already maintains the allotted maximum
     * number of IPv6 data connections.
     */
    MAX_IPV6_CONNECTIONS = 0x805,
    /**
     * New PDN bring up is rejected during interface selection because the UE has already allotted
     * the available interfaces for other PDNs.
     */
    APN_MISMATCH = 0x806,
    /**
     * New call bring up is rejected since the existing data call IP type doesn't match the
     * requested IP.
     */
    IP_VERSION_MISMATCH = 0x807,
    /**
     * Dial up networking (DUN) call bring up is rejected since UE is in eHRPD RAT.
     */
    DUN_CALL_DISALLOWED = 0x808,
    /**
     * Rejected/Brought down since UE is transition between EPC and NONEPC RAT.
     */
    INTERNAL_EPC_NONEPC_TRANSITION = 0x809,
    /**
     * The current interface is being in use.
     */
    INTERFACE_IN_USE = 0x80A,
    /**
     * PDN connection to the APN is disallowed on the roaming network.
     */
    APN_DISALLOWED_ON_ROAMING = 0x80B,
    /**
     * APN-related parameters are changed.
     */
    APN_PARAMETERS_CHANGED = 0x80C,
    /**
     * PDN is attempted to be brought up with NULL APN but NULL APN is not supported.
     */
    NULL_APN_DISALLOWED = 0x80D,
    /**
     * Thermal level increases and causes calls to be torn down when normal mode of operation is
     * not allowed.
     */
    THERMAL_MITIGATION = 0x80E,
    /**
     * PDN Connection to a given APN is disallowed because data is disabled from the device user
     * interface settings.
     */
    DATA_SETTINGS_DISABLED = 0x80F,
    /**
     * PDN Connection to a given APN is disallowed because data roaming is disabled from the device
     * user interface settings and the UE is roaming.
     */
    DATA_ROAMING_SETTINGS_DISABLED = 0x810,
    /**
     * DDS (Default data subscription) switch occurs.
     */
    DDS_SWITCHED = 0x811,
    /**
     * PDN being brought up with an APN that is part of forbidden APN Name list.
     */
    FORBIDDEN_APN_NAME = 0x812,
    /**
     * Default data subscription switch is in progress.
     */
    DDS_SWITCH_IN_PROGRESS = 0x813,
    /**
     * Roaming is disallowed during call bring up.
     */
    CALL_DISALLOWED_IN_ROAMING = 0x814,
    /**
     * UE is unable to bring up a non-IP data call because the device is not camped on a NB1 cell.
     */
    NON_IP_NOT_SUPPORTED = 0x815,
    /**
     * Non-IP PDN is in throttled state due to previous VSNCP bringup failure(s).
     */
    PDN_NON_IP_CALL_THROTTLED = 0x816,
    /**
     * Non-IP PDN is in disallowed state due to the network providing only an IP address.
     */
    PDN_NON_IP_CALL_DISALLOWED = 0x817,
    /**
     * Device in CDMA locked state.
     */
    CDMA_LOCK = 0x818,
    /**
     * Received an intercept order from the base station.
     */
    CDMA_INTERCEPT = 0x819,
    /**
     * Receiving a reorder from the base station.
     */
    CDMA_REORDER = 0x81A,
    /**
     * Receiving a release from the base station with a SO (Service Option) Reject reason.
     */
    CDMA_RELEASE_DUE_TO_SO_REJECTION = 0x81B,
    /**
     * Receiving an incoming call from the base station.
     */
    CDMA_INCOMING_CALL = 0x81C,
    /**
     * Received an alert stop from the base station due to incoming only.
     */
    CDMA_ALERT_STOP = 0x81D,
    /**
     * Channel acquisition failures. This indicates that device has failed acquiring all the
     * channels in the PRL.
     */
    CHANNEL_ACQUISITION_FAILURE = 0x81E,
    /**
     * Maximum access probes transmitted.
     */
    MAX_ACCESS_PROBE = 0x81F,
    /**
     * Concurrent service is not supported by base station.
     */
    CONCURRENT_SERVICE_NOT_SUPPORTED_BY_BASE_STATION = 0x820,
    /**
     * There was no response received from the base station.
     */
    NO_RESPONSE_FROM_BASE_STATION = 0x821,
    /**
     * The base station rejecting the call.
     */
    REJECTED_BY_BASE_STATION = 0x822,
    /**
     * The concurrent services requested were not compatible.
     */
    CONCURRENT_SERVICES_INCOMPATIBLE = 0x823,
    /**
     * Device does not have CDMA service.
     */
    NO_CDMA_SERVICE = 0x824,
    /**
     * RUIM not being present.
     */
    RUIM_NOT_PRESENT = 0x825,
    /**
     * Receiving a retry order from the base station.
     */
    CDMA_RETRY_ORDER = 0x826,
    /**
     * Access blocked by the base station.
     */
    ACCESS_BLOCK = 0x827,
    /**
     * Access blocked by the base station for all mobile devices.
     */
    ACCESS_BLOCK_ALL = 0x828,
    /**
     * Maximum access probes for the IS-707B call.
     */
    IS707B_MAX_ACCESS_PROBES = 0x829,
    /**
     * Put device in thermal emergency.
     */
    THERMAL_EMERGENCY = 0x82A,
    /**
     * In favor of a voice call or SMS when concurrent voice and data are not supported.
     */
    CONCURRENT_SERVICES_NOT_ALLOWED = 0x82B,
    /**
     * The other clients rejected incoming call.
     */
    INCOMING_CALL_REJECTED = 0x82C,
    /**
     * No service on the gateway.
     */
    NO_SERVICE_ON_GATEWAY = 0x82D,
    /**
     * GPRS context is not available.
     */
    NO_GPRS_CONTEXT = 0x82E,
    /**
     * Network refuses service to the MS because either an identity of the MS is not acceptable to
     * the network or the MS does not pass the authentication check.
     */
    ILLEGAL_MS = 0x82F,
    /**
     * ME could not be authenticated and the ME used is not acceptable to the network.
     */
    ILLEGAL_ME = 0x830,
    /**
     * Not allowed to operate either GPRS or non-GPRS services.
     */
    GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED = 0x831,
    /**
     * MS is not allowed to operate GPRS services.
     */
    GPRS_SERVICES_NOT_ALLOWED = 0x832,
    /**
     * No matching identity or context could be found in the network.
     */
    MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK = 0x833,
    /**
     * Mobile reachable timer has expired, or the GMM context data related to the subscription does
     * not exist in the SGSN.
     */
    IMPLICITLY_DETACHED = 0x834,
    /**
     * UE requests GPRS service, or the network initiates a detach request in a PLMN which does not
     * offer roaming for GPRS services to that MS.
     */
    PLMN_NOT_ALLOWED = 0x835,
    /**
     * MS requests service, or the network initiates a detach request, in a location area where the
     * HPLMN determines that the MS, by subscription, is not allowed to operate.
     */
    LOCATION_AREA_NOT_ALLOWED = 0x836,
    /**
     * UE requests GPRS service or the network initiates a detach request in a PLMN that does not
     * offer roaming for GPRS services.
     */
    GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN = 0x837,
    /**
     * PDP context already exists.
     */
    PDP_DUPLICATE = 0x838,
    /**
     * RAT change on the UE.
     */
    UE_RAT_CHANGE = 0x839,
    /**
     * Network cannot serve a request from the MS due to congestion.
     */
    CONGESTION = 0x83A,
    /**
     * MS requests an establishment of the radio access bearers for all active PDP contexts by
     * sending a service request message indicating data to the network, but the SGSN does not have
     * any active PDP context.
     */
    NO_PDP_CONTEXT_ACTIVATED = 0x83B,
    /**
     * Access class blocking restrictions for the current camped cell.
     */
    ACCESS_CLASS_DSAC_REJECTION = 0x83C,
    /**
     * SM attempts PDP activation for a maximum of four attempts.
     */
    PDP_ACTIVATE_MAX_RETRY_FAILED = 0x83D,
    /**
     * Radio access bearer failure.
     */
    RADIO_ACCESS_BEARER_FAILURE = 0x83E,
    /**
     * Invalid EPS bearer identity in the request.
     */
    ESM_UNKNOWN_EPS_BEARER_CONTEXT = 0x83F,
    /**
     * Data radio bearer is released by the RRC.
     */
    DRB_RELEASED_BY_RRC = 0x840,
    /**
     * Indicate the connection was released.
     */
    CONNECTION_RELEASED = 0x841,
    /**
     * UE is detached.
     */
    EMM_DETACHED = 0x842,
    /**
     * Attach procedure is rejected by the network.
     */
    EMM_ATTACH_FAILED = 0x843,
    /**
     * Attach procedure is started for EMC purposes.
     */
    EMM_ATTACH_STARTED = 0x844,
    /**
     * Service request procedure failure.
     */
    LTE_NAS_SERVICE_REQUEST_FAILED = 0x845,
    /**
     * Active dedicated bearer was requested using the same default bearer ID.
     */
    DUPLICATE_BEARER_ID = 0x846,
    /**
     * Collision scenarios for the UE and network-initiated procedures.
     */
    ESM_COLLISION_SCENARIOS = 0x847,
    /**
     * Bearer must be deactivated to synchronize with the network.
     */
    ESM_BEARER_DEACTIVATED_TO_SYNC_WITH_NETWORK = 0x848,
    /**
     * Active dedicated bearer was requested for an existing default bearer.
     */
    ESM_NW_ACTIVATED_DED_BEARER_WITH_ID_OF_DEF_BEARER = 0x849,
    /**
     * Bad OTA message is received from the network.
     */
    ESM_BAD_OTA_MESSAGE = 0x84A,
    /**
     * Download server rejected the call.
     */
    ESM_DOWNLOAD_SERVER_REJECTED_THE_CALL = 0x84B,
    /**
     * PDN was disconnected by the downlaod server due to IRAT.
     */
    ESM_CONTEXT_TRANSFERRED_DUE_TO_IRAT = 0x84C,
    /**
     * Dedicated bearer will be deactivated regardless of the network response.
     */
    DS_EXPLICIT_DEACTIVATION = 0x84D,
    /**
     * No specific local cause is mentioned, usually a valid OTA cause.
     */
    ESM_LOCAL_CAUSE_NONE = 0x84E,
    /**
     * Throttling is not needed for this service request failure.
     */
    LTE_THROTTLING_NOT_REQUIRED = 0x84F,
    /**
     * Access control list check failure at the lower layer.
     */
    ACCESS_CONTROL_LIST_CHECK_FAILURE = 0x850,
    /**
     * Service is not allowed on the requested PLMN.
     */
    SERVICE_NOT_ALLOWED_ON_PLMN = 0x851,
    /**
     * T3417 timer expiration of the service request procedure.
     */
    EMM_T3417_EXPIRED = 0x852,
    /**
     * Extended service request fails due to expiration of the T3417 EXT timer.
     */
    EMM_T3417_EXT_EXPIRED = 0x853,
    /**
     * Transmission failure of radio resource control (RRC) uplink data.
     */
    RRC_UPLINK_DATA_TRANSMISSION_FAILURE = 0x854,
    /**
     * Radio resource control (RRC) uplink data delivery failed due to a handover.
     */
    RRC_UPLINK_DELIVERY_FAILED_DUE_TO_HANDOVER = 0x855,
    /**
     * Radio resource control (RRC) uplink data delivery failed due to a connection release.
     */
    RRC_UPLINK_CONNECTION_RELEASE = 0x856,
    /**
     * Radio resource control (RRC) uplink data delivery failed due to a radio link failure.
     */
    RRC_UPLINK_RADIO_LINK_FAILURE = 0x857,
    /**
     * Radio resource control (RRC) is not connected but the non-access stratum (NAS) sends an
     * uplink data request.
     */
    RRC_UPLINK_ERROR_REQUEST_FROM_NAS = 0x858,
    /**
     * Radio resource control (RRC) connection failure at access stratum.
     */
    RRC_CONNECTION_ACCESS_STRATUM_FAILURE = 0x859,
    /**
     * Radio resource control (RRC) connection establishment is aborted due to another procedure.
     */
    RRC_CONNECTION_ANOTHER_PROCEDURE_IN_PROGRESS = 0x85A,
    /**
     * Radio resource control (RRC) connection establishment failed due to access barrred.
     */
    RRC_CONNECTION_ACCESS_BARRED = 0x85B,
    /**
     * Radio resource control (RRC) connection establishment failed due to cell reselection at
     * access stratum.
     */
    RRC_CONNECTION_CELL_RESELECTION = 0x85C,
    /**
     * Connection establishment failed due to configuration failure at the radio resource control
     * (RRC).
     */
    RRC_CONNECTION_CONFIG_FAILURE = 0x85D,
    /**
     * Radio resource control (RRC) connection could not be established in the time limit.
     */
    RRC_CONNECTION_TIMER_EXPIRED = 0x85E,
    /**
     * Connection establishment failed due to a link failure at the radio resource control (RRC).
     */
    RRC_CONNECTION_LINK_FAILURE = 0x85F,
    /**
     * Connection establishment failed as the radio resource control (RRC) is not camped on any
     * cell.
     */
    RRC_CONNECTION_CELL_NOT_CAMPED = 0x860,
    /**
     * Connection establishment failed due to a service interval failure at the radio resource
     * control (RRC).
     */
    RRC_CONNECTION_SYSTEM_INTERVAL_FAILURE = 0x861,
    /**
     * Radio resource control (RRC) connection establishment failed due to the network rejecting the
     * UE connection request.
     */
    RRC_CONNECTION_REJECT_BY_NETWORK = 0x862,
    /**
     * Normal radio resource control (RRC) connection release.
     */
    RRC_CONNECTION_NORMAL_RELEASE = 0x863,
    /**
     * Radio resource control (RRC) connection release failed due to radio link failure conditions.
     */
    RRC_CONNECTION_RADIO_LINK_FAILURE = 0x864,
    /**
     * Radio resource control (RRC) connection re-establishment failure.
     */
    RRC_CONNECTION_REESTABLISHMENT_FAILURE = 0x865,
    /**
     * UE is out of service during the call register.
     */
    RRC_CONNECTION_OUT_OF_SERVICE_DURING_CELL_REGISTER = 0x866,
    /**
     * Connection has been released by the radio resource control (RRC) due to an abort request.
     */
    RRC_CONNECTION_ABORT_REQUEST = 0x867,
    /**
     * Radio resource control (RRC) connection released due to a system information block read
     * error.
     */
    RRC_CONNECTION_SYSTEM_INFORMATION_BLOCK_READ_ERROR = 0x868,
    /**
     * Network-initiated detach with reattach.
     */
    NETWORK_INITIATED_DETACH_WITH_AUTO_REATTACH = 0x869,
    /**
     * Network-initiated detach without reattach.
     */
    NETWORK_INITIATED_DETACH_NO_AUTO_REATTACH = 0x86A,
    /**
     * ESM procedure maximum attempt timeout failure.
     */
    ESM_PROCEDURE_TIME_OUT = 0x86B,
    /**
     * No PDP exists with the given connection ID while modifying or deactivating or activation for
     * an already active PDP.
     */
    INVALID_CONNECTION_ID = 0x86C,
    /**
     * Maximum NSAPIs have been exceeded during PDP activation.
     */
    MAXIMIUM_NSAPIS_EXCEEDED = 0x86D,
    /**
     * Primary context for NSAPI does not exist.
     */
    INVALID_PRIMARY_NSAPI = 0x86E,
    /**
     * Unable to encode the OTA message for MT PDP or deactivate PDP.
     */
    CANNOT_ENCODE_OTA_MESSAGE = 0x86F,
    /**
     * Radio access bearer is not established by the lower layers during activation, modification,
     * or deactivation.
     */
    RADIO_ACCESS_BEARER_SETUP_FAILURE = 0x870,
    /**
     * Expiration of the PDP establish timer with a maximum of five retries.
     */
    PDP_ESTABLISH_TIMEOUT_EXPIRED = 0x871,
    /**
     * Expiration of the PDP modify timer with a maximum of four retries.
     */
    PDP_MODIFY_TIMEOUT_EXPIRED = 0x872,
    /**
     * Expiration of the PDP deactivate timer with a maximum of four retries.
     */
    PDP_INACTIVE_TIMEOUT_EXPIRED = 0x873,
    /**
     * PDP activation failed due to RRC_ABORT or a forbidden PLMN.
     */
    PDP_LOWERLAYER_ERROR = 0x874,
    /**
     * MO PDP modify collision when the MT PDP is already in progress.
     */
    PDP_MODIFY_COLLISION = 0x875,
    /**
     * @deprecated use MAXIMUM_SIZE_OF_L2_MESSAGE_EXCEEDED instead.
     */
    MAXINUM_SIZE_OF_L2_MESSAGE_EXCEEDED = 0x876,
    /**
     * Maximum size of the L2 message was exceeded.
     */
    MAXIMUM_SIZE_OF_L2_MESSAGE_EXCEEDED = 0x876,
    /**
     * Non-access stratum (NAS) request was rejected by the network.
     */
    NAS_REQUEST_REJECTED_BY_NETWORK = 0x877,
    /**
     * Radio resource control (RRC) connection establishment failure due to an error in the request
     * message.
     */
    RRC_CONNECTION_INVALID_REQUEST = 0x878,
    /**
     * Radio resource control (RRC) connection establishment failure due to a change in the tracking
     * area ID.
     */
    RRC_CONNECTION_TRACKING_AREA_ID_CHANGED = 0x879,
    /**
     * Radio resource control (RRC) connection establishment failure due to the RF was unavailable.
     */
    RRC_CONNECTION_RF_UNAVAILABLE = 0x87A,
    /**
     * Radio resource control (RRC) connection was aborted before deactivating the LTE stack due to
     * a successful LTE to WCDMA/GSM/TD-SCDMA IRAT change.
     */
    RRC_CONNECTION_ABORTED_DUE_TO_IRAT_CHANGE = 0x87B,
    /**
     * If the UE has an LTE radio link failure before security is established, the radio resource
     * control (RRC) connection must be released and the UE must return to idle.
     */
    RRC_CONNECTION_RELEASED_SECURITY_NOT_ACTIVE = 0x87C,
    /**
     * Radio resource control (RRC) connection was aborted by the non-access stratum (NAS) after an
     * IRAT to LTE IRAT handover.
     */
    RRC_CONNECTION_ABORTED_AFTER_HANDOVER = 0x87D,
    /**
     * Radio resource control (RRC) connection was aborted before deactivating the LTE stack after a
     * successful LTE to GSM/EDGE IRAT cell change order procedure.
     */
    RRC_CONNECTION_ABORTED_AFTER_IRAT_CELL_CHANGE = 0x87E,
    /**
     * Radio resource control (RRC) connection was aborted in the middle of a LTE to GSM IRAT cell
     * change order procedure.
     */
    RRC_CONNECTION_ABORTED_DURING_IRAT_CELL_CHANGE = 0x87F,
    /**
     * IMSI present in the UE is unknown in the home subscriber server.
     */
    IMSI_UNKNOWN_IN_HOME_SUBSCRIBER_SERVER = 0x880,
    /**
     * IMEI of the UE is not accepted by the network.
     */
    IMEI_NOT_ACCEPTED = 0x881,
    /**
     * EPS and non-EPS services are not allowed by the network.
     */
    EPS_SERVICES_AND_NON_EPS_SERVICES_NOT_ALLOWED = 0x882,
    /**
     * EPS services are not allowed in the PLMN.
     */
    EPS_SERVICES_NOT_ALLOWED_IN_PLMN = 0x883,
    /**
     * Mobile switching center is temporarily unreachable.
     */
    MSC_TEMPORARILY_NOT_REACHABLE = 0x884,
    /**
     * CS domain is not available.
     */
    CS_DOMAIN_NOT_AVAILABLE = 0x885,
    /**
     * ESM level failure.
     */
    ESM_FAILURE = 0x886,
    /**
     * MAC level failure.
     */
    MAC_FAILURE = 0x887,
    /**
     * Synchronization failure.
     */
    SYNCHRONIZATION_FAILURE = 0x888,
    /**
     * UE security capabilities mismatch.
     */
    UE_SECURITY_CAPABILITIES_MISMATCH = 0x889,
    /**
     * Unspecified security mode reject.
     */
    SECURITY_MODE_REJECTED = 0x88A,
    /**
     * Unacceptable non-EPS authentication.
     */
    UNACCEPTABLE_NON_EPS_AUTHENTICATION = 0x88B,
    /**
     * CS fallback call establishment is not allowed.
     */
    CS_FALLBACK_CALL_ESTABLISHMENT_NOT_ALLOWED = 0x88C,
    /**
     * No EPS bearer context was activated.
     */
    NO_EPS_BEARER_CONTEXT_ACTIVATED = 0x88D,
    /**
     * Invalid EMM state.
     */
    INVALID_EMM_STATE = 0x88E,
    /**
     * Non-Access Spectrum layer failure.
     */
    NAS_LAYER_FAILURE = 0x88F,
    /**
     * Multiple PDP call feature is disabled.
     */
    MULTIPLE_PDP_CALL_NOT_ALLOWED = 0x890,
    /**
     * Data call has been brought down because EMBMS is not enabled at the RRC layer.
     */
    EMBMS_NOT_ENABLED = 0x891,
    /**
     * Data call was unsuccessfully transferred during the IRAT handover.
     */
    IRAT_HANDOVER_FAILED = 0x892,
    /**
     * EMBMS data call has been successfully brought down.
     */
    EMBMS_REGULAR_DEACTIVATION = 0x893,
    /**
     * Test loop-back data call has been successfully brought down.
     */
    TEST_LOOPBACK_REGULAR_DEACTIVATION = 0x894,
    /**
     * Lower layer registration failure.
     */
    LOWER_LAYER_REGISTRATION_FAILURE = 0x895,
    /**
     * Network initiates a detach on LTE with error cause "data plan has been replenished or has
     * expired".
     */
    DATA_PLAN_EXPIRED = 0x896,
    /**
     * UMTS interface is brought down due to handover from UMTS to iWLAN.
     */
    UMTS_HANDOVER_TO_IWLAN = 0x897,
    /**
     * Received a connection deny due to general or network busy on EVDO network.
     */
    EVDO_CONNECTION_DENY_BY_GENERAL_OR_NETWORK_BUSY = 0x898,
    /**
     * Received a connection deny due to billing or authentication failure on EVDO network.
     */
    EVDO_CONNECTION_DENY_BY_BILLING_OR_AUTHENTICATION_FAILURE = 0x899,
    /**
     * HDR system has been changed due to redirection or the PRL was not preferred.
     */
    EVDO_HDR_CHANGED = 0x89A,
    /**
     * Device exited HDR due to redirection or the PRL was not preferred.
     */
    EVDO_HDR_EXITED = 0x89B,
    /**
     * Device does not have an HDR session.
     */
    EVDO_HDR_NO_SESSION = 0x89C,
    /**
     * It is ending an HDR call origination in favor of a GPS fix.
     */
    EVDO_USING_GPS_FIX_INSTEAD_OF_HDR_CALL = 0x89D,
    /**
     * Connection setup on the HDR system was time out.
     */
    EVDO_HDR_CONNECTION_SETUP_TIMEOUT = 0x89E,
    /**
     * Device failed to acquire a co-located HDR for origination.
     */
    FAILED_TO_ACQUIRE_COLOCATED_HDR = 0x89F,
    /**
     * OTASP commit is in progress.
     */
    OTASP_COMMIT_IN_PROGRESS = 0x8A0,
    /**
     * Device has no hybrid HDR service.
     */
    NO_HYBRID_HDR_SERVICE = 0x8A1,
    /**
     * HDR module could not be obtained because of the RF locked.
     */
    HDR_NO_LOCK_GRANTED = 0x8A2,
    /**
     * DBM or SMS is in progress.
     */
    DBM_OR_SMS_IN_PROGRESS = 0x8A3,
    /**
     * HDR module released the call due to fade.
     */
    HDR_FADE = 0x8A4,
    /**
     * HDR system access failure.
     */
    HDR_ACCESS_FAILURE = 0x8A5,
    /**
     * P_rev supported by 1 base station is less than 6, which is not supported for a 1X data call.
     * The UE must be in the footprint of BS which has p_rev >= 6 to support this SO33 call.
     */
    UNSUPPORTED_1X_PREV = 0x8A6,
    /**
     * Client ended the data call.
     */
    LOCAL_END = 0x8A7,
    /**
     * Device has no service.
     */
    NO_SERVICE = 0x8A8,
    /**
     * Device lost the system due to fade.
     */
    FADE = 0x8A9,
    /**
     * Receiving a release from the base station with no reason.
     */
    NORMAL_RELEASE = 0x8AA,
    /**
     * Access attempt is already in progress.
     */
    ACCESS_ATTEMPT_ALREADY_IN_PROGRESS = 0x8AB,
    /**
     * Device is in the process of redirecting or handing off to a different target system.
     */
    REDIRECTION_OR_HANDOFF_IN_PROGRESS = 0x8AC,
    /**
     * Device is operating in Emergency mode.
     */
    EMERGENCY_MODE = 0x8AD,
    /**
     * Device is in use (e.g., voice call).
     */
    PHONE_IN_USE = 0x8AE,
    /**
     * Device operational mode is different from the mode requested in the traffic channel bring up.
     */
    INVALID_MODE = 0x8AF,
    /**
     * SIM was marked by the network as invalid for the circuit and/or packet service domain.
     */
    INVALID_SIM_STATE = 0x8B0,
    /**
     * There is no co-located HDR.
     */
    NO_COLLOCATED_HDR = 0x8B1,
    /**
     * UE is entering power save mode.
     */
    UE_IS_ENTERING_POWERSAVE_MODE = 0x8B2,
    /**
     * Dual switch from single standby to dual standby is in progress.
     */
    DUAL_SWITCH = 0x8B3,
    /**
     * Data call bring up fails in the PPP setup due to a timeout. (e.g., an LCP conf ack was not
     * received from the network)
     */
    PPP_TIMEOUT = 0x8B4,
    /**
     * Data call bring up fails in the PPP setup due to an authorization failure.
     * (e.g., authorization is required, but not negotiated with the network during an LCP phase)
     */
    PPP_AUTH_FAILURE = 0x8B5,
    /**
     * Data call bring up fails in the PPP setup due to an option mismatch.
     */
    PPP_OPTION_MISMATCH = 0x8B6,
    /**
     * Data call bring up fails in the PPP setup due to a PAP failure.
     */
    PPP_PAP_FAILURE = 0x8B7,
    /**
     * Data call bring up fails in the PPP setup due to a CHAP failure.
     */
    PPP_CHAP_FAILURE = 0x8B8,
    /**
     * Data call bring up fails in the PPP setup because the PPP is in the process of cleaning the
     * previous PPP session.
     */
    PPP_CLOSE_IN_PROGRESS = 0x8B9,
    /**
     * IPv6 interface bring up fails because the network provided only the IPv4 address for the
     * upcoming PDN permanent client can reattempt a IPv6 call bring up after the IPv4 interface is
     * also brought down. However, there is no guarantee that the network will provide a IPv6
     * address.
     */
    LIMITED_TO_IPV4 = 0x8BA,
    /**
     * IPv4 interface bring up fails because the network provided only the IPv6 address for the
     * upcoming PDN permanent client can reattempt a IPv4 call bring up after the IPv6 interface is
     * also brought down. However there is no guarantee that the network will provide a IPv4
     * address.
     */
    LIMITED_TO_IPV6 = 0x8BB,
    /**
     * Data call bring up fails in the VSNCP phase due to a VSNCP timeout error.
     */
    VSNCP_TIMEOUT = 0x8BC,
    /**
     * Data call bring up fails in the VSNCP phase due to a general error. It's used when there is
     * no other specific error code available to report the failure.
     */
    VSNCP_GEN_ERROR = 0x8BD,
    /**
     * Data call bring up fails in the VSNCP phase due to a network rejection of the VSNCP
     * configuration request because the requested APN is unauthorized.
     */
    VSNCP_APN_UNAUTHORIZED = 0x8BE,
    /**
     * Data call bring up fails in the VSNCP phase due to a network rejection of the VSNCP
     * configuration request because the PDN limit has been exceeded.
     */
    VSNCP_PDN_LIMIT_EXCEEDED = 0x8BF,
    /**
     * Data call bring up fails in the VSNCP phase due to the network rejected the VSNCP
     * configuration request due to no PDN gateway address.
     */
    VSNCP_NO_PDN_GATEWAY_ADDRESS = 0x8C0,
    /**
     * Data call bring up fails in the VSNCP phase due to a network rejection of the VSNCP
     * configuration request because the PDN gateway is unreachable.
     */
    VSNCP_PDN_GATEWAY_UNREACHABLE = 0x8C1,
    /**
     * Data call bring up fails in the VSNCP phase due to a network rejection of the VSNCP
     * configuration request due to a PDN gateway reject.
     */
    VSNCP_PDN_GATEWAY_REJECT = 0x8C2,
    /**
     * Data call bring up fails in the VSNCP phase due to a network rejection of the VSNCP
     * configuration request with the reason of insufficient parameter.
     */
    VSNCP_INSUFFICIENT_PARAMETERS = 0x8C3,
    /**
     * Data call bring up fails in the VSNCP phase due to a network rejection of the VSNCP
     * configuration request with the reason of resource unavailable.
     */
    VSNCP_RESOURCE_UNAVAILABLE = 0x8C4,
    /**
     * Data call bring up fails in the VSNCP phase due to a network rejection of the VSNCP
     * configuration request with the reason of administratively prohibited at the HSGW.
     */
    VSNCP_ADMINISTRATIVELY_PROHIBITED = 0x8C5,
    /**
     * Data call bring up fails in the VSNCP phase due to a network rejection of PDN ID in use, or
     * all existing PDNs are brought down with this end reason because one of the PDN bring up was
     * rejected by the network with the reason of PDN ID in use.
     */
    VSNCP_PDN_ID_IN_USE = 0x8C6,
    /**
     * Data call bring up fails in the VSNCP phase due to a network rejection of the VSNCP
     * configuration request for the reason of subscriber limitation.
     */
    VSNCP_SUBSCRIBER_LIMITATION = 0x8C7,
    /**
     * Data call bring up fails in the VSNCP phase due to a network rejection of the VSNCP
     * configuration request because the PDN exists for this APN.
     */
    VSNCP_PDN_EXISTS_FOR_THIS_APN = 0x8C8,
    /**
     * Data call bring up fails in the VSNCP phase due to a network rejection of the VSNCP
     * configuration request with reconnect to this PDN not allowed, or an active data call is
     * terminated by the network because reconnection to this PDN is not allowed. Upon receiving
     * this error code from the network, the modem infinitely throttles the PDN until the next power
     * cycle.
     */
    VSNCP_RECONNECT_NOT_ALLOWED = 0x8C9,
    /**
     * Device failure to obtain the prefix from the network.
     */
    IPV6_PREFIX_UNAVAILABLE = 0x8CA,
    /**
     * System preference change back to SRAT during handoff
     */
    HANDOFF_PREFERENCE_CHANGED = 0x8CB,
    /**
     * Data call fail due to the slice not being allowed for the data call.
     */
    SLICE_REJECTED = 0x8CC,
    /**
     * No matching rule available for the request, and match-all rule is not allowed for it.
     */
    MATCH_ALL_RULE_NOT_ALLOWED = 0x8CD,
    /**
     * If connection failed for all matching URSP rules.
     */
    ALL_MATCHING_RULES_FAILED = 0x8CE,
}
