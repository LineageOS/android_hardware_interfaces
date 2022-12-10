/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __WIFI_HAL_H__
#define __WIFI_HAL_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#define IFNAMSIZ 16

/* typedefs */
typedef unsigned char byte;
typedef unsigned char u8;
typedef signed char s8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef int wifi_request_id;
typedef int wifi_channel;  // indicates channel frequency in MHz
typedef int wifi_rssi;
typedef int wifi_radio;
typedef byte mac_addr[6];
typedef byte oui[3];
typedef int64_t wifi_timestamp;  // In microseconds (us)
typedef int64_t wifi_timespan;   // In picoseconds  (ps)
typedef uint64_t feature_set;

/* forward declarations */
struct wifi_info;
struct wifi_interface_info;
typedef struct wifi_info* wifi_handle;
typedef struct wifi_interface_info* wifi_interface_handle;

/* WiFi Common definitions */
/* channel operating width */
typedef enum {
    WIFI_CHAN_WIDTH_20 = 0,
    WIFI_CHAN_WIDTH_40 = 1,
    WIFI_CHAN_WIDTH_80 = 2,
    WIFI_CHAN_WIDTH_160 = 3,
    WIFI_CHAN_WIDTH_80P80 = 4,
    WIFI_CHAN_WIDTH_5 = 5,
    WIFI_CHAN_WIDTH_10 = 6,
    WIFI_CHAN_WIDTH_320 = 7,
    WIFI_CHAN_WIDTH_INVALID = -1
} wifi_channel_width;

/* Pre selected Power scenarios to be applied from BDF file */
typedef enum {
    WIFI_POWER_SCENARIO_INVALID = -2,
    WIFI_POWER_SCENARIO_DEFAULT = -1,
    WIFI_POWER_SCENARIO_VOICE_CALL = 0,
    WIFI_POWER_SCENARIO_ON_HEAD_CELL_OFF = 1,
    WIFI_POWER_SCENARIO_ON_HEAD_CELL_ON = 2,
    WIFI_POWER_SCENARIO_ON_BODY_CELL_OFF = 3,
    WIFI_POWER_SCENARIO_ON_BODY_CELL_ON = 4,
    WIFI_POWER_SCENARIO_ON_BODY_BT = 5,
    WIFI_POWER_SCENARIO_ON_HEAD_HOTSPOT = 6,
    WIFI_POWER_SCENARIO_ON_HEAD_HOTSPOT_MMW = 7,
    WIFI_POWER_SCENARIO_ON_BODY_CELL_ON_BT = 8,
    WIFI_POWER_SCENARIO_ON_BODY_HOTSPOT = 9,
    WIFI_POWER_SCENARIO_ON_BODY_HOTSPOT_BT = 10,
    WIFI_POWER_SCENARIO_ON_BODY_HOTSPOT_MMW = 11,
    WIFI_POWER_SCENARIO_ON_BODY_HOTSPOT_BT_MMW = 12,
    WIFI_POWER_SCENARIO_ON_HEAD_CELL_OFF_UNFOLDED = 13,
    WIFI_POWER_SCENARIO_ON_HEAD_CELL_ON_UNFOLDED = 14,
    WIFI_POWER_SCENARIO_ON_HEAD_HOTSPOT_UNFOLDED = 15,
    WIFI_POWER_SCENARIO_ON_HEAD_HOTSPOT_MMW_UNFOLDED = 16,
    WIFI_POWER_SCENARIO_ON_BODY_CELL_OFF_UNFOLDED = 17,
    WIFI_POWER_SCENARIO_ON_BODY_BT_UNFOLDED = 18,
    WIFI_POWER_SCENARIO_ON_BODY_CELL_ON_UNFOLDED = 19,
    WIFI_POWER_SCENARIO_ON_BODY_CELL_ON_BT_UNFOLDED = 20,
    WIFI_POWER_SCENARIO_ON_BODY_HOTSPOT_UNFOLDED = 21,
    WIFI_POWER_SCENARIO_ON_BODY_HOTSPOT_BT_UNFOLDED = 22,
    WIFI_POWER_SCENARIO_ON_BODY_HOTSPOT_MMW_UNFOLDED = 23,
    WIFI_POWER_SCENARIO_ON_BODY_HOTSPOT_BT_MMW_UNFOLDED = 24,
} wifi_power_scenario;

typedef enum {
    WIFI_LATENCY_MODE_NORMAL = 0,
    WIFI_LATENCY_MODE_LOW = 1,
} wifi_latency_mode;

/* Wifi Thermal mitigation modes */
typedef enum {
    WIFI_MITIGATION_NONE = 0,
    WIFI_MITIGATION_LIGHT = 1,
    WIFI_MITIGATION_MODERATE = 2,
    WIFI_MITIGATION_SEVERE = 3,
    WIFI_MITIGATION_CRITICAL = 4,
    WIFI_MITIGATION_EMERGENCY = 5,
} wifi_thermal_mode;

/*
 * Wifi voice over IP mode
 * may add new modes later, for example, voice + video over IP mode.
 */
typedef enum {
    WIFI_VOIP_MODE_OFF = 0,
    WIFI_VOIP_MODE_ON = 1,
} wifi_voip_mode;

/* List of interface types supported */
typedef enum {
    WIFI_INTERFACE_TYPE_STA = 0,
    WIFI_INTERFACE_TYPE_AP = 1,
    WIFI_INTERFACE_TYPE_P2P = 2,
    WIFI_INTERFACE_TYPE_NAN = 3,
} wifi_interface_type;

/*
 * enum wlan_mac_band - Band information corresponding to the WLAN MAC.
 */
typedef enum {
    /* WLAN MAC Operates in 2.4 GHz Band */
    WLAN_MAC_2_4_BAND = 1 << 0,
    /* WLAN MAC Operates in 5 GHz Band */
    WLAN_MAC_5_0_BAND = 1 << 1,
    /* WLAN MAC Operates in 6 GHz Band */
    WLAN_MAC_6_0_BAND = 1 << 2,
    /* WLAN MAC Operates in 60 GHz Band */
    WLAN_MAC_60_0_BAND = 1 << 3,
} wlan_mac_band;

/* List of chre nan rtt state */
typedef enum {
    CHRE_PREEMPTED = 0,
    CHRE_UNAVAILABLE = 1,
    CHRE_AVAILABLE = 2,
} chre_nan_rtt_state;

typedef struct {
    wifi_channel_width width;
    int center_frequency0;
    int center_frequency1;
    int primary_frequency;
} wifi_channel_spec;

/*
 * wifi_usable_channel specifies a channel frequency, bandwidth, and bitmask
 * of modes allowed on the channel.
 */
typedef struct {
    /* Channel frequency in MHz */
    wifi_channel freq;
    /* Channel operating width (20, 40, 80, 160, 320 etc.) */
    wifi_channel_width width;
    /* BIT MASK of BIT(WIFI_INTERFACE_*) represented by |wifi_interface_mode|
     * Bitmask does not represent concurrency.
     * Examples:
     * - If a channel is usable only for STA, then only the WIFI_INTERFACE_STA
     *   bit would be set for that channel.
     * - If 5GHz SAP is not allowed, then none of the 5GHz channels will have
     *   WIFI_INTERFACE_SOFTAP bit set.
     * Note: TDLS bit is set only if there is a STA connection. TDLS bit is set
     * on non-STA channels only if TDLS off channel is supported.
     */
    u32 iface_mode_mask;
} wifi_usable_channel;

/*
 * wifi_usable_channel_filter
 */
typedef enum {
    /* Filter Wifi channels that should be avoided due to cellular coex
     * restrictions. Some Wifi channels can have extreme interference
     * from/to cellular due to short frequency separation with neighboring
     * cellular channels or when there is harmonic and intermodulation
     * interference. Channels which only have some performance degradation
     * (e.g. power back off is sufficient to deal with coexistence issue)
     * can be included and should not be filtered out.
     */
    WIFI_USABLE_CHANNEL_FILTER_CELLULAR_COEXISTENCE = 1 << 0,
    /* Filter channels due to concurrency state.
     * Examples:
     * - 5GHz SAP operation may be supported in standalone mode, but if
     *  there is STA connection on 5GHz DFS channel, none of the 5GHz
     *  channels are usable for SAP if device does not support DFS SAP mode.
     * - P2P GO may not be supported on indoor channels in EU during
     *  standalone mode but if there is a STA connection on indoor channel,
     *  P2P GO may be supported by some vendors on the same STA channel.
     */
    WIFI_USABLE_CHANNEL_FILTER_CONCURRENCY = 1 << 1,
    /* This Filter queries Wifi channels and bands that are supported for
     * NAN3.1 Instant communication mode. This filter should only be applied to NAN interface.
     * If 5G is supported default discovery channel 149/44 is considered,
     * If 5G is not supported then channel 6 has to be considered.
     * Based on regulatory domain if channel 149 and 44 are restricted, channel 6 should
     * be considered for instant communication channel
     */
    WIFI_USABLE_CHANNEL_FILTER_NAN_INSTANT_MODE = 1 << 2,
} wifi_usable_channel_filter;

typedef enum {
    WIFI_SUCCESS = 0,
    WIFI_ERROR_NONE = 0,
    WIFI_ERROR_UNKNOWN = -1,
    WIFI_ERROR_UNINITIALIZED = -2,
    WIFI_ERROR_NOT_SUPPORTED = -3,
    WIFI_ERROR_NOT_AVAILABLE = -4,  // Not available right now, but try later
    WIFI_ERROR_INVALID_ARGS = -5,
    WIFI_ERROR_INVALID_REQUEST_ID = -6,
    WIFI_ERROR_TIMED_OUT = -7,
    WIFI_ERROR_TOO_MANY_REQUESTS = -8,  // Too many instances of this request
    WIFI_ERROR_OUT_OF_MEMORY = -9,
    WIFI_ERROR_BUSY = -10,
} wifi_error;

typedef enum {
    WIFI_ACCESS_CATEGORY_BEST_EFFORT = 0,
    WIFI_ACCESS_CATEGORY_BACKGROUND = 1,
    WIFI_ACCESS_CATEGORY_VIDEO = 2,
    WIFI_ACCESS_CATEGORY_VOICE = 3
} wifi_access_category;

/* Antenna configuration */
typedef enum {
    WIFI_ANTENNA_UNSPECIFIED = 0,
    WIFI_ANTENNA_1X1 = 1,
    WIFI_ANTENNA_2X2 = 2,
    WIFI_ANTENNA_3X3 = 3,
    WIFI_ANTENNA_4X4 = 4,
} wifi_antenna_configuration;

/* Wifi Radio configuration */
typedef struct {
    /* Operating band */
    wlan_mac_band band;
    /* Antenna configuration */
    wifi_antenna_configuration antenna_cfg;
} wifi_radio_configuration;

/* WiFi Radio Combination  */
typedef struct {
    u32 num_radio_configurations;
    wifi_radio_configuration radio_configurations[];
} wifi_radio_combination;

/* WiFi Radio combinations matrix */
/* For Example in case of a chip which has two radios, where one radio is
 * capable of 2.4GHz 2X2 only and another radio which is capable of either
 * 5GHz or 6GHz 2X2, number of possible radio combinations in this case
 * are 5 and possible combinations are
 *                            {{{2G 2X2}}, //Standalone 2G
 *                            {{5G 2X2}}, //Standalone 5G
 *                            {{6G 2X2}}, //Standalone 6G
 *                            {{2G 2X2}, {5G 2X2}}, //2G+5G DBS
 *                            {{2G 2X2}, {6G 2X2}}} //2G+6G DBS
 * Note: Since this chip doesn’t support 5G+6G simultaneous operation
 * as there is only one radio which can support both, So it can only
 * do MCC 5G+6G. This table should not get populated with possible MCC
 * configurations. This is only for simultaneous radio configurations
 * (such as Standalone, multi band simultaneous or single band simultaneous).
 */
typedef struct {
    u32 num_radio_combinations;
    /* Each row represents possible radio combinations */
    wifi_radio_combination radio_combinations[];
} wifi_radio_combination_matrix;

/* Initialize/Cleanup */

wifi_error wifi_initialize(wifi_handle* handle);

/**
 * wifi_wait_for_driver
 * Function should block until the driver is ready to proceed.
 * Any errors from this function is considered fatal & will fail the HAL startup sequence.
 *
 * on success returns WIFI_SUCCESS
 * on failure returns WIFI_ERROR_TIMED_OUT
 */
wifi_error wifi_wait_for_driver_ready(void);

typedef void (*wifi_cleaned_up_handler)(wifi_handle handle);
void wifi_cleanup(wifi_handle handle, wifi_cleaned_up_handler handler);
void wifi_event_loop(wifi_handle handle);

/* Error handling */
void wifi_get_error_info(wifi_error err, const char** msg);  // return a pointer to a static string

/* Feature enums */
#define WIFI_FEATURE_INFRA (uint64_t)0x1                   // Basic infrastructure mode
#define WIFI_FEATURE_INFRA_5G (uint64_t)0x2                // Support for 5 GHz Band
#define WIFI_FEATURE_HOTSPOT (uint64_t)0x4                 // Support for GAS/ANQP
#define WIFI_FEATURE_P2P (uint64_t)0x8                     // Wifi-Direct
#define WIFI_FEATURE_SOFT_AP (uint64_t)0x10                // Soft AP
#define WIFI_FEATURE_GSCAN (uint64_t)0x20                  // Google-Scan APIs
#define WIFI_FEATURE_NAN (uint64_t)0x40                    // Neighbor Awareness Networking
#define WIFI_FEATURE_D2D_RTT (uint64_t)0x80                // Device-to-device RTT
#define WIFI_FEATURE_D2AP_RTT (uint64_t)0x100              // Device-to-AP RTT
#define WIFI_FEATURE_BATCH_SCAN (uint64_t)0x200            // Batched Scan (legacy)
#define WIFI_FEATURE_PNO (uint64_t)0x400                   // Preferred network offload
#define WIFI_FEATURE_ADDITIONAL_STA (uint64_t)0x800        // Support for two STAs
#define WIFI_FEATURE_TDLS (uint64_t)0x1000                 // Tunnel directed link setup
#define WIFI_FEATURE_TDLS_OFFCHANNEL (uint64_t)0x2000      // Support for TDLS off channel
#define WIFI_FEATURE_EPR (uint64_t)0x4000                  // Enhanced power reporting
#define WIFI_FEATURE_AP_STA (uint64_t)0x8000               // Support for AP STA Concurrency
#define WIFI_FEATURE_LINK_LAYER_STATS (uint64_t)0x10000    // Link layer stats collection
#define WIFI_FEATURE_LOGGER (uint64_t)0x20000              // WiFi Logger
#define WIFI_FEATURE_HAL_EPNO (uint64_t)0x40000            // WiFi PNO enhanced
#define WIFI_FEATURE_RSSI_MONITOR (uint64_t)0x80000        // RSSI Monitor
#define WIFI_FEATURE_MKEEP_ALIVE (uint64_t)0x100000        // WiFi mkeep_alive
#define WIFI_FEATURE_CONFIG_NDO (uint64_t)0x200000         // ND offload configure
#define WIFI_FEATURE_TX_TRANSMIT_POWER (uint64_t)0x400000  // Capture Tx transmit power levels
#define WIFI_FEATURE_CONTROL_ROAMING (uint64_t)0x800000    // Enable/Disable firmware roaming
#define WIFI_FEATURE_IE_WHITELIST (uint64_t)0x1000000      // Support Probe IE white listing
#define WIFI_FEATURE_SCAN_RAND \
    (uint64_t)0x2000000  // Support MAC & Probe Sequence Number randomization
#define WIFI_FEATURE_SET_TX_POWER_LIMIT (uint64_t)0x4000000  // Support Tx Power Limit setting
#define WIFI_FEATURE_USE_BODY_HEAD_SAR \
    (uint64_t)0x8000000  // Support Using Body/Head Proximity for SAR
#define WIFI_FEATURE_DYNAMIC_SET_MAC \
    (uint64_t)0x10000000  // Support changing MAC address without iface reset(down and up)
#define WIFI_FEATURE_SET_LATENCY_MODE (uint64_t)0x40000000  // Support Latency mode setting
#define WIFI_FEATURE_P2P_RAND_MAC (uint64_t)0x80000000      // Support P2P MAC randomization
#define WIFI_FEATURE_INFRA_60G (uint64_t)0x100000000        // Support for 60GHz Band
// Add more features here

#define IS_MASK_SET(mask, flags) (((flags) & (mask)) == (mask))

#define IS_SUPPORTED_FEATURE(feature, featureSet) IS_MASK_SET(feature, featureSet)

/* Feature set */
wifi_error wifi_get_supported_feature_set(wifi_interface_handle handle, feature_set* set);

/*
 * Each row represents a valid feature combination;
 * all other combinations are invalid!
 */
wifi_error wifi_get_concurrency_matrix(wifi_interface_handle handle, int set_size_max,
                                       feature_set set[], int* set_size);

/* multiple interface support */

wifi_error wifi_get_ifaces(wifi_handle handle, int* num_ifaces, wifi_interface_handle** ifaces);
wifi_error wifi_get_iface_name(wifi_interface_handle iface, char* name, size_t size);
wifi_interface_handle wifi_get_iface_handle(wifi_handle handle, char* name);

/* STA + STA support - Supported if WIFI_FEATURE_ADDITIONAL_STA is set */

/**
 * Invoked to indicate that the provided iface is the primary STA iface when there are more
 * than 1 STA iface concurrently active.
 *
 * Note: If the wifi firmware/chip cannot support multiple instances of any offload
 * (like roaming, APF, rssi threshold, etc), the firmware should ensure that these
 * offloads are at least enabled for the primary interface. If the new primary interface is
 * already connected to a network, the firmware must switch all the offloads on
 * this new interface without disconnecting.
 */
wifi_error wifi_multi_sta_set_primary_connection(wifi_handle handle, wifi_interface_handle iface);

/**
 * When there are 2 or more simultaneous STA connections, this use case hint indicates what
 * use-case is being enabled by the framework. This use case hint can be used by the firmware
 * to modify various firmware configurations like:
 *  - Allowed BSSIDs the firmware can choose for the initial connection/roaming attempts.
 *  - Duty cycle to choose for the 2 STA connections if the radio is in MCC mode.
 *  - Whether roaming, APF and other offloads needs to be enabled or not.
 *
 * Note:
 *  - This will be invoked before an active wifi connection is established on the second interface.
 *  - This use-case hint is implicitly void when the second STA interface is brought down.
 */
typedef enum {
    /**
     * Usage:
     * - This will be sent down for make before break use-case.
     * - Platform is trying to speculatively connect to a second network and evaluate it without
     *   disrupting the primary connection.
     *
     * Requirements for Firmware:
     * - Do not reduce the number of tx/rx chains of primary connection.
     * - If using MCC, should set the MCC duty cycle of the primary connection to be higher than
     *   the secondary connection (maybe 70/30 split).
     * - Should pick the best BSSID for the secondary STA (disregard the chip mode) independent of
     *   the primary STA:
     *     - Don’t optimize for DBS vs MCC/SCC
     * - Should not impact the primary connection’s bssid selection:
     *     - Don’t downgrade chains of the existing primary connection.
     *     - Don’t optimize for DBS vs MCC/SCC.
     */
    WIFI_DUAL_STA_TRANSIENT_PREFER_PRIMARY = 0,
    /**
     * Usage:
     * - This will be sent down for any app requested peer to peer connections.
     * - In this case, both the connections needs to be allocated equal resources.
     * - For the peer to peer use case, BSSID for the secondary connection will be chosen by the
     *   framework.
     *
     * Requirements for Firmware:
     * - Can choose MCC or DBS mode depending on the MCC efficiency and HW capability.
     * - If using MCC, set the MCC duty cycle of the primary connection to be equal to the secondary
     *   connection.
     * - Prefer BSSID candidates which will help provide the best "overall" performance for both the
     *   connections.
     */
    WIFI_DUAL_STA_NON_TRANSIENT_UNBIASED = 1
} wifi_multi_sta_use_case;

wifi_error wifi_multi_sta_set_use_case(wifi_handle handle, wifi_multi_sta_use_case use_case);

/* Configuration events */

typedef struct {
    void (*on_country_code_changed)(char code[2]);  // We can get this from supplicant too

    // More event handlers
} wifi_event_handler;

typedef struct {
    char iface_name[IFNAMSIZ + 1];
    wifi_channel channel;
} wifi_iface_info;

typedef struct {
    u32 wlan_mac_id;
    /* BIT MASK of BIT(WLAN_MAC*) as represented by wlan_mac_band */
    u32 mac_band;
    /* Represents the connected Wi-Fi interfaces associated with each MAC */
    int num_iface;
    wifi_iface_info* iface_info;
} wifi_mac_info;

typedef struct {
    void (*on_radio_mode_change)(wifi_request_id id, unsigned num_mac, wifi_mac_info* mac_info);
} wifi_radio_mode_change_handler;

typedef struct {
    void (*on_rssi_threshold_breached)(wifi_request_id id, u8* cur_bssid, s8 cur_rssi);
} wifi_rssi_event_handler;

typedef struct {
    void (*on_subsystem_restart)(const char* error);
} wifi_subsystem_restart_handler;

typedef struct {
    void (*on_chre_nan_rtt_change)(chre_nan_rtt_state state);
} wifi_chre_handler;

wifi_error wifi_set_iface_event_handler(wifi_request_id id, wifi_interface_handle iface,
                                        wifi_event_handler eh);
wifi_error wifi_reset_iface_event_handler(wifi_request_id id, wifi_interface_handle iface);

wifi_error wifi_set_nodfs_flag(wifi_interface_handle handle, u32 nodfs);
wifi_error wifi_select_tx_power_scenario(wifi_interface_handle handle,
                                         wifi_power_scenario scenario);
wifi_error wifi_reset_tx_power_scenario(wifi_interface_handle handle);
wifi_error wifi_set_latency_mode(wifi_interface_handle handle, wifi_latency_mode mode);
wifi_error wifi_map_dscp_access_category(wifi_handle handle, uint32_t start, uint32_t end,
                                         uint32_t access_category);
wifi_error wifi_reset_dscp_mapping(wifi_handle handle);

wifi_error wifi_set_subsystem_restart_handler(wifi_handle handle,
                                              wifi_subsystem_restart_handler handler);

/**
 *  Wifi HAL Thermal Mitigation API
 *
 *  wifi_handle : wifi global handle (note: this is not a interface specific
 *  command). Mitigation is expected to be applied across all active interfaces
 *  The implementation and the mitigation action mapping to each mode is chip
 *  specific. Mitigation will be active until Wifi is turned off or
 *  WIFI_MITIGATION_NONE mode is sent
 *
 *  mode: Thermal mitigation mode
 *  WIFI_MITIGATION_NONE     : Clear all Wifi thermal mitigation actions
 *  WIFI_MITIGATION_LIGHT    : Light Throttling where UX is not impacted
 *  WIFI_MITIGATION_MODERATE : Moderate throttling where UX not largely impacted
 *  WIFI_MITIGATION_SEVERE   : Severe throttling where UX is largely impacted
 *  WIFI_MITIGATION_CRITICAL : Platform has done everything to reduce power
 *  WIFI_MITIGATION_EMERGENCY: Key components in platform are shutting down
 *
 *  completion_window
 *  Deadline (in milliseconds) to complete this request, value 0 implies apply
 *  immediately. Deadline is basically a relaxed limit and allows vendors to
 *  apply the mitigation within the window (if it cannot apply immediately)
 *
 *  Return
 *  WIFI_ERROR_NOT_SUPPORTED : Chip does not support thermal mitigation
 *  WIFI_ERROR_BUSY          : Mitigation is supported, but retry later
 *  WIFI_ERROR_NONE          : Mitigation request has been accepted
 */
wifi_error wifi_set_thermal_mitigation_mode(wifi_handle handle, wifi_thermal_mode mode,
                                            u32 completion_window);

typedef struct rx_data_cnt_details_t {
    int rx_unicast_cnt;   /*Total rx unicast packet which woke up host */
    int rx_multicast_cnt; /*Total rx multicast packet which woke up host */
    int rx_broadcast_cnt; /*Total rx broadcast packet which woke up host */
} RX_DATA_WAKE_CNT_DETAILS;

typedef struct rx_wake_pkt_type_classification_t {
    int icmp_pkt;  /*wake icmp packet count */
    int icmp6_pkt; /*wake icmp6 packet count */
    int icmp6_ra;  /*wake icmp6 RA packet count */
    int icmp6_na;  /*wake icmp6 NA packet count */
    int icmp6_ns;  /*wake icmp6 NS packet count */
    // ToDo: Any more interesting classification to add?
} RX_WAKE_PKT_TYPE_CLASSFICATION;

typedef struct rx_multicast_cnt_t {
    int ipv4_rx_multicast_addr_cnt;  /*Rx wake packet was ipv4 multicast */
    int ipv6_rx_multicast_addr_cnt;  /*Rx wake packet was ipv6 multicast */
    int other_rx_multicast_addr_cnt; /*Rx wake packet was non-ipv4 and non-ipv6*/
} RX_MULTICAST_WAKE_DATA_CNT;

/*
 * Structure holding all the driver/firmware wake count reasons.
 *
 * Buffers for the array fields (cmd_event_wake_cnt/driver_fw_local_wake_cnt)
 * are allocated and freed by the framework. The size of each allocated
 * array is indicated by the corresponding |_cnt| field. HAL needs to fill in
 * the corresponding |_used| field to indicate the number of elements used in
 * the array.
 */
typedef struct wlan_driver_wake_reason_cnt_t {
    int total_cmd_event_wake;    /* Total count of cmd event wakes */
    int* cmd_event_wake_cnt;     /* Individual wake count array, each index a reason */
    int cmd_event_wake_cnt_sz;   /* Max number of cmd event wake reasons */
    int cmd_event_wake_cnt_used; /* Number of cmd event wake reasons specific to the driver */

    int total_driver_fw_local_wake;    /* Total count of drive/fw wakes, for local reasons */
    int* driver_fw_local_wake_cnt;     /* Individual wake count array, each index a reason */
    int driver_fw_local_wake_cnt_sz;   /* Max number of local driver/fw wake reasons */
    int driver_fw_local_wake_cnt_used; /* Number of local driver/fw wake reasons specific to the
                                          driver */

    int total_rx_data_wake; /* total data rx packets, that woke up host */
    RX_DATA_WAKE_CNT_DETAILS rx_wake_details;
    RX_WAKE_PKT_TYPE_CLASSFICATION rx_wake_pkt_classification_info;
    RX_MULTICAST_WAKE_DATA_CNT rx_multicast_wake_pkt_info;
} WLAN_DRIVER_WAKE_REASON_CNT;

/* Wi-Fi coex channel avoidance support */

#define WIFI_COEX_NO_POWER_CAP (int32_t)0x7FFFFFF

typedef enum { WIFI_AWARE = 1 << 0, SOFTAP = 1 << 1, WIFI_DIRECT = 1 << 2 } wifi_coex_restriction;

/**
 * Representation of a Wi-Fi channel to be avoided for Wi-Fi coex channel avoidance.
 *
 * band is represented as an WLAN_MAC* enum value defined in wlan_mac_band.
 * If power_cap_dbm is WIFI_COEX_NO_POWER_CAP, then no power cap should be applied if the specified
 * channel is used.
 */
typedef struct {
    wlan_mac_band band;
    u32 channel;
    s32 power_cap_dbm;
} wifi_coex_unsafe_channel;

/* include various feature headers */

#include "gscan.h"
#include "link_layer_stats.h"
#include "roam.h"
#include "rtt.h"
#include "tdls.h"
#include "wifi_cached_scan_results.h"
#include "wifi_config.h"
#include "wifi_logger.h"
#include "wifi_nan.h"
#include "wifi_offload.h"
#include "wifi_twt.h"

// wifi HAL function pointer table
typedef struct {
    wifi_error (*wifi_initialize)(wifi_handle*);
    wifi_error (*wifi_wait_for_driver_ready)(void);
    void (*wifi_cleanup)(wifi_handle, wifi_cleaned_up_handler);
    void (*wifi_event_loop)(wifi_handle);
    void (*wifi_get_error_info)(wifi_error, const char**);
    wifi_error (*wifi_get_supported_feature_set)(wifi_interface_handle, feature_set*);
    wifi_error (*wifi_get_concurrency_matrix)(wifi_interface_handle, int, feature_set*, int*);
    wifi_error (*wifi_set_scanning_mac_oui)(wifi_interface_handle, unsigned char*);
    wifi_error (*wifi_get_supported_channels)(wifi_handle, int*, wifi_channel*);
    wifi_error (*wifi_is_epr_supported)(wifi_handle);
    wifi_error (*wifi_get_ifaces)(wifi_handle, int*, wifi_interface_handle**);
    wifi_error (*wifi_get_iface_name)(wifi_interface_handle, char* name, size_t);
    wifi_error (*wifi_set_iface_event_handler)(wifi_request_id, wifi_interface_handle,
                                               wifi_event_handler);
    wifi_error (*wifi_reset_iface_event_handler)(wifi_request_id, wifi_interface_handle);
    wifi_error (*wifi_start_gscan)(wifi_request_id, wifi_interface_handle, wifi_scan_cmd_params,
                                   wifi_scan_result_handler);
    wifi_error (*wifi_stop_gscan)(wifi_request_id, wifi_interface_handle);
    wifi_error (*wifi_get_cached_gscan_results)(wifi_interface_handle, byte, int,
                                                wifi_cached_scan_results*, int*);
    wifi_error (*wifi_set_bssid_hotlist)(wifi_request_id, wifi_interface_handle,
                                         wifi_bssid_hotlist_params, wifi_hotlist_ap_found_handler);
    wifi_error (*wifi_reset_bssid_hotlist)(wifi_request_id, wifi_interface_handle);
    wifi_error (*wifi_set_significant_change_handler)(wifi_request_id, wifi_interface_handle,
                                                      wifi_significant_change_params,
                                                      wifi_significant_change_handler);
    wifi_error (*wifi_reset_significant_change_handler)(wifi_request_id, wifi_interface_handle);
    wifi_error (*wifi_get_gscan_capabilities)(wifi_interface_handle, wifi_gscan_capabilities*);
    wifi_error (*wifi_set_link_stats)(wifi_interface_handle, wifi_link_layer_params);
    wifi_error (*wifi_get_link_stats)(wifi_request_id, wifi_interface_handle,
                                      wifi_stats_result_handler);
    wifi_error (*wifi_clear_link_stats)(wifi_interface_handle, u32, u32*, u8, u8*);
    wifi_error (*wifi_get_valid_channels)(wifi_interface_handle, int, int, wifi_channel*, int*);
    wifi_error (*wifi_rtt_range_request)(wifi_request_id, wifi_interface_handle, unsigned,
                                         wifi_rtt_config[], wifi_rtt_event_handler);
    wifi_error (*wifi_rtt_range_cancel)(wifi_request_id, wifi_interface_handle, unsigned,
                                        mac_addr[]);
    wifi_error (*wifi_get_rtt_capabilities)(wifi_interface_handle, wifi_rtt_capabilities*);
    wifi_error (*wifi_rtt_get_responder_info)(wifi_interface_handle iface,
                                              wifi_rtt_responder* responder_info);
    wifi_error (*wifi_enable_responder)(wifi_request_id id, wifi_interface_handle iface,
                                        wifi_channel_info channel_hint,
                                        unsigned max_duration_seconds,
                                        wifi_rtt_responder* responder_info);
    wifi_error (*wifi_disable_responder)(wifi_request_id id, wifi_interface_handle iface);
    wifi_error (*wifi_set_nodfs_flag)(wifi_interface_handle, u32);
    wifi_error (*wifi_start_logging)(wifi_interface_handle, u32, u32, u32, u32, char*);
    wifi_error (*wifi_set_epno_list)(wifi_request_id, wifi_interface_handle,
                                     const wifi_epno_params*, wifi_epno_handler);
    wifi_error (*wifi_reset_epno_list)(wifi_request_id, wifi_interface_handle);
    wifi_error (*wifi_set_country_code)(wifi_interface_handle, const char*);
    wifi_error (*wifi_get_firmware_memory_dump)(wifi_interface_handle iface,
                                                wifi_firmware_memory_dump_handler handler);
    wifi_error (*wifi_set_log_handler)(wifi_request_id id, wifi_interface_handle iface,
                                       wifi_ring_buffer_data_handler handler);
    wifi_error (*wifi_reset_log_handler)(wifi_request_id id, wifi_interface_handle iface);
    wifi_error (*wifi_set_alert_handler)(wifi_request_id id, wifi_interface_handle iface,
                                         wifi_alert_handler handler);
    wifi_error (*wifi_reset_alert_handler)(wifi_request_id id, wifi_interface_handle iface);
    wifi_error (*wifi_get_firmware_version)(wifi_interface_handle iface, char* buffer,
                                            int buffer_size);
    wifi_error (*wifi_get_ring_buffers_status)(wifi_interface_handle iface, u32* num_rings,
                                               wifi_ring_buffer_status* status);
    wifi_error (*wifi_get_logger_supported_feature_set)(wifi_interface_handle iface,
                                                        unsigned int* support);
    wifi_error (*wifi_get_ring_data)(wifi_interface_handle iface, char* ring_name);
    wifi_error (*wifi_enable_tdls)(wifi_interface_handle, mac_addr, wifi_tdls_params*,
                                   wifi_tdls_handler);
    wifi_error (*wifi_disable_tdls)(wifi_interface_handle, mac_addr);
    wifi_error (*wifi_get_tdls_status)(wifi_interface_handle, mac_addr, wifi_tdls_status*);
    wifi_error (*wifi_get_tdls_capabilities)(wifi_interface_handle iface,
                                             wifi_tdls_capabilities* capabilities);
    wifi_error (*wifi_get_driver_version)(wifi_interface_handle iface, char* buffer,
                                          int buffer_size);
    wifi_error (*wifi_set_passpoint_list)(wifi_request_id id, wifi_interface_handle iface, int num,
                                          wifi_passpoint_network* networks,
                                          wifi_passpoint_event_handler handler);
    wifi_error (*wifi_reset_passpoint_list)(wifi_request_id id, wifi_interface_handle iface);
    wifi_error (*wifi_set_lci)(wifi_request_id id, wifi_interface_handle iface,
                               wifi_lci_information* lci);
    wifi_error (*wifi_set_lcr)(wifi_request_id id, wifi_interface_handle iface,
                               wifi_lcr_information* lcr);
    wifi_error (*wifi_start_sending_offloaded_packet)(wifi_request_id id,
                                                      wifi_interface_handle iface, u16 ether_type,
                                                      u8* ip_packet, u16 ip_packet_len,
                                                      u8* src_mac_addr, u8* dst_mac_addr,
                                                      u32 period_msec);
    wifi_error (*wifi_stop_sending_offloaded_packet)(wifi_request_id id,
                                                     wifi_interface_handle iface);
    wifi_error (*wifi_start_rssi_monitoring)(wifi_request_id id, wifi_interface_handle iface,
                                             s8 max_rssi, s8 min_rssi, wifi_rssi_event_handler eh);
    wifi_error (*wifi_stop_rssi_monitoring)(wifi_request_id id, wifi_interface_handle iface);
    wifi_error (*wifi_get_wake_reason_stats)(wifi_interface_handle iface,
                                             WLAN_DRIVER_WAKE_REASON_CNT* wifi_wake_reason_cnt);
    wifi_error (*wifi_configure_nd_offload)(wifi_interface_handle iface, u8 enable);
    wifi_error (*wifi_get_driver_memory_dump)(wifi_interface_handle iface,
                                              wifi_driver_memory_dump_callbacks callbacks);
    wifi_error (*wifi_start_pkt_fate_monitoring)(wifi_interface_handle iface);
    wifi_error (*wifi_get_tx_pkt_fates)(wifi_interface_handle handle,
                                        wifi_tx_report* tx_report_bufs, size_t n_requested_fates,
                                        size_t* n_provided_fates);
    wifi_error (*wifi_get_rx_pkt_fates)(wifi_interface_handle handle,
                                        wifi_rx_report* rx_report_bufs, size_t n_requested_fates,
                                        size_t* n_provided_fates);

    /* NAN functions */
    wifi_error (*wifi_nan_enable_request)(transaction_id id, wifi_interface_handle iface,
                                          NanEnableRequest* msg);
    wifi_error (*wifi_nan_disable_request)(transaction_id id, wifi_interface_handle iface);
    wifi_error (*wifi_nan_publish_request)(transaction_id id, wifi_interface_handle iface,
                                           NanPublishRequest* msg);
    wifi_error (*wifi_nan_publish_cancel_request)(transaction_id id, wifi_interface_handle iface,
                                                  NanPublishCancelRequest* msg);
    wifi_error (*wifi_nan_subscribe_request)(transaction_id id, wifi_interface_handle iface,
                                             NanSubscribeRequest* msg);
    wifi_error (*wifi_nan_subscribe_cancel_request)(transaction_id id, wifi_interface_handle iface,
                                                    NanSubscribeCancelRequest* msg);
    wifi_error (*wifi_nan_transmit_followup_request)(transaction_id id, wifi_interface_handle iface,
                                                     NanTransmitFollowupRequest* msg);
    wifi_error (*wifi_nan_stats_request)(transaction_id id, wifi_interface_handle iface,
                                         NanStatsRequest* msg);
    wifi_error (*wifi_nan_config_request)(transaction_id id, wifi_interface_handle iface,
                                          NanConfigRequest* msg);
    wifi_error (*wifi_nan_tca_request)(transaction_id id, wifi_interface_handle iface,
                                       NanTCARequest* msg);
    wifi_error (*wifi_nan_beacon_sdf_payload_request)(transaction_id id,
                                                      wifi_interface_handle iface,
                                                      NanBeaconSdfPayloadRequest* msg);
    wifi_error (*wifi_nan_register_handler)(wifi_interface_handle iface,
                                            NanCallbackHandler handlers);
    wifi_error (*wifi_nan_get_version)(wifi_handle handle, NanVersion* version);
    wifi_error (*wifi_nan_get_capabilities)(transaction_id id, wifi_interface_handle iface);
    wifi_error (*wifi_nan_data_interface_create)(transaction_id id, wifi_interface_handle iface,
                                                 char* iface_name);
    wifi_error (*wifi_nan_data_interface_delete)(transaction_id id, wifi_interface_handle iface,
                                                 char* iface_name);
    wifi_error (*wifi_nan_data_request_initiator)(transaction_id id, wifi_interface_handle iface,
                                                  NanDataPathInitiatorRequest* msg);
    wifi_error (*wifi_nan_data_indication_response)(transaction_id id, wifi_interface_handle iface,
                                                    NanDataPathIndicationResponse* msg);
    wifi_error (*wifi_nan_data_end)(transaction_id id, wifi_interface_handle iface,
                                    NanDataPathEndRequest* msg);
    wifi_error (*wifi_select_tx_power_scenario)(wifi_interface_handle iface,
                                                wifi_power_scenario scenario);
    wifi_error (*wifi_reset_tx_power_scenario)(wifi_interface_handle iface);

    /**
     * Returns the chipset's hardware filtering capabilities:
     * @param version pointer to version of the packet filter interpreter
     *                supported, filled in upon return. 0 indicates no support.
     * @param max_len pointer to maximum size of the filter bytecode, filled in
     *                upon return.
     */
    wifi_error (*wifi_get_packet_filter_capabilities)(wifi_interface_handle handle, u32* version,
                                                      u32* max_len);
    /**
     * Programs the packet filter.
     * @param program pointer to the program byte-code.
     * @param len length of the program byte-code.
     */
    wifi_error (*wifi_set_packet_filter)(wifi_interface_handle handle, const u8* program, u32 len);
    wifi_error (*wifi_read_packet_filter)(wifi_interface_handle handle, u32 src_offset,
                                          u8* host_dst, u32 length);
    wifi_error (*wifi_get_roaming_capabilities)(wifi_interface_handle handle,
                                                wifi_roaming_capabilities* caps);
    wifi_error (*wifi_enable_firmware_roaming)(wifi_interface_handle handle,
                                               fw_roaming_state_t state);
    wifi_error (*wifi_configure_roaming)(wifi_interface_handle handle,
                                         wifi_roaming_config* roaming_config);
    wifi_error (*wifi_set_radio_mode_change_handler)(wifi_request_id id,
                                                     wifi_interface_handle iface,
                                                     wifi_radio_mode_change_handler eh);
    wifi_error (*wifi_set_latency_mode)(wifi_interface_handle iface, wifi_latency_mode mode);
    wifi_error (*wifi_set_thermal_mitigation_mode)(wifi_handle handle, wifi_thermal_mode mode,
                                                   u32 completion_window);
    wifi_error (*wifi_map_dscp_access_category)(wifi_handle handle, u32 start, u32 end,
                                                u32 access_category);
    wifi_error (*wifi_reset_dscp_mapping)(wifi_handle handle);

    wifi_error (*wifi_virtual_interface_create)(wifi_handle handle, const char* ifname,
                                                wifi_interface_type iface_type);
    wifi_error (*wifi_virtual_interface_delete)(wifi_handle handle, const char* ifname);

    wifi_error (*wifi_set_subsystem_restart_handler)(wifi_handle handle,
                                                     wifi_subsystem_restart_handler handler);

    /**
     * Allow vendor HAL to choose interface name when creating
     * an interface. This can be implemented by chips with their
     * own interface naming policy.
     * If not implemented, the default naming will be used.
     */
    wifi_error (*wifi_get_supported_iface_name)(wifi_handle handle, u32 iface_type, char* name,
                                                size_t len);

    /**
     * Perform early initialization steps that are needed when WIFI
     * is disabled.
     * If the function returns failure, it means the vendor HAL is unusable
     * (for example, if chip hardware is not installed) and no further
     * functions should be called.
     */
    wifi_error (*wifi_early_initialize)(void);

    /**
     * Get supported feature set which are chip-global, that is
     * not dependent on any created interface.
     */
    wifi_error (*wifi_get_chip_feature_set)(wifi_handle handle, feature_set* set);

    /**
     * Invoked to indicate that the provided iface is the primary STA iface when there are more
     * than 1 STA iface concurrently active.
     */
    wifi_error (*wifi_multi_sta_set_primary_connection)(wifi_handle handle,
                                                        wifi_interface_handle iface);

    /**
     * When there are 2 simultaneous STA connections, this use case hint
     * indicates what STA + STA use-case is being enabled by the framework.
     */
    wifi_error (*wifi_multi_sta_set_use_case)(wifi_handle handle, wifi_multi_sta_use_case use_case);

    /**
     * Invoked to indicate that the following list of wifi_coex_unsafe_channel should be avoided
     * with the specified restrictions.
     * @param unsafeChannels list of current |wifi_coex_unsafe_channel| to avoid.
     * @param restrictions bitmask of |wifi_coex_restriction| indicating wifi interfaces to
     *         restrict from the current unsafe channels.
     */
    wifi_error (*wifi_set_coex_unsafe_channels)(wifi_handle handle, u32 num_channels,
                                                wifi_coex_unsafe_channel* unsafeChannels,
                                                u32 restrictions);

    /**
     * Invoked to set voip optimization mode for the provided STA iface
     */
    wifi_error (*wifi_set_voip_mode)(wifi_interface_handle iface, wifi_voip_mode mode);

    /**@brief twt_register_handler
     *        Request to register TWT callback before sending any TWT request
     * @param wifi_interface_handle:
     * @param TwtCallbackHandler: callback function pointers
     * @return Synchronous wifi_error
     */
    wifi_error (*wifi_twt_register_handler)(wifi_interface_handle iface,
                                            TwtCallbackHandler handler);

    /**@brief twt_get_capability
     *        Request TWT capability
     * @param wifi_interface_handle:
     * @return Synchronous wifi_error and TwtCapabilitySet
     */
    wifi_error (*wifi_twt_get_capability)(wifi_interface_handle iface,
                                          TwtCapabilitySet* twt_cap_set);

    /**@brief twt_setup_request
     *        Request to send TWT setup frame
     * @param wifi_interface_handle:
     * @param TwtSetupRequest: detailed parameters of setup request
     * @return Synchronous wifi_error
     * @return Asynchronous EventTwtSetupResponse CB return TwtSetupResponse
     */
    wifi_error (*wifi_twt_setup_request)(wifi_interface_handle iface, TwtSetupRequest* msg);

    /**@brief twt_teardown_request
     *        Request to send TWT teardown frame
     * @param wifi_interface_handle:
     * @param TwtTeardownRequest: detailed parameters of teardown request
     * @return Synchronous wifi_error
     * @return Asynchronous EventTwtTeardownCompletion CB return TwtTeardownCompletion
     * TwtTeardownCompletion may also be received due to other events
     * like CSA, BTCX, TWT scheduler, MultiConnection, peer-initiated teardown, etc.
     */
    wifi_error (*wifi_twt_teardown_request)(wifi_interface_handle iface, TwtTeardownRequest* msg);

    /**@brief twt_info_frame_request
     *        Request to send TWT info frame
     * @param wifi_interface_handle:
     * @param TwtInfoFrameRequest: detailed parameters in info frame
     * @return Synchronous wifi_error
     * @return Asynchronous EventTwtInfoFrameReceived CB return TwtInfoFrameReceived
     * Driver may also receive Peer-initiated TwtInfoFrame
     */
    wifi_error (*wifi_twt_info_frame_request)(wifi_interface_handle iface,
                                              TwtInfoFrameRequest* msg);

    /**@brief twt_get_stats
     *        Request to get TWT stats
     * @param wifi_interface_handle:
     * @param config_id: configuration ID of TWT request
     * @return Synchronous wifi_error and TwtStats
     */
    wifi_error (*wifi_twt_get_stats)(wifi_interface_handle iface, u8 config_id, TwtStats* stats);

    /**@brief twt_clear_stats
     *        Request to clear TWT stats
     * @param wifi_interface_handle:
     * @param config_id: configuration ID of TWT request
     * @return Synchronous wifi_error
     */
    wifi_error (*wifi_twt_clear_stats)(wifi_interface_handle iface, u8 config_id);

    /**
     * Invoked to set DTIM configuration when the host is in the suspend mode
     * @param wifi_interface_handle:
     * @param multiplier: when STA in the power saving mode, the wake up interval will be set to
     *              1) multiplier * DTIM period if multiplier > 0.
     *              2) the device default value if multiplier <=0
     * Some implementations may apply an additional cap to wake up interval in the case of 1).
     */
    wifi_error (*wifi_set_dtim_config)(wifi_interface_handle handle, u32 multiplier);

    /**@brief wifi_get_usable_channels
     *        Request list of usable channels for the requested bands and modes. Usable
     *        implies channel is allowed as per regulatory for the current country code
     *        and not restricted due to other hard limitations (e.g. DFS, Coex) In
     *        certain modes (e.g. STA+SAP) there could be other hard restrictions
     *        since MCC operation many not be supported by SAP. This API also allows
     *        driver to return list of usable channels for each mode uniquely to
     *        distinguish cases where only a limited set of modes are allowed on
     *        a given channel e.g. srd channels may be supported for P2P but not
     *        for SAP or P2P-Client may be allowed on an indoor channel but P2P-GO
     *        may not be allowed. This API is not interface specific and will be
     *        used to query capabilities of driver in terms of what modes (STA, SAP,
     *        P2P_CLI, P2P_GO, NAN, TDLS) can be supported on each of the channels.
     * @param handle global wifi_handle
     * @param band_mask BIT MASK of WLAN_MAC* as represented by |wlan_mac_band|
     * @param iface_mode_mask BIT MASK of BIT(WIFI_INTERFACE_*) represented by
     *        |wifi_interface_mode|. Bitmask respresents all the modes that the
     *        caller is interested in (e.g. STA, SAP, WFD-CLI, WFD-GO, TDLS, NAN).
     *        Note: Bitmask does not represent concurrency matrix. If the caller
     *        is interested in CLI, GO modes, the iface_mode_mask would be set
     *        to WIFI_INTERFACE_P2P_CLIENT|WIFI_INTERFACE_P2P_GO.
     * @param filter_mask BIT MASK of WIFI_USABLE_CHANNEL_FILTER_* represented by
     *        |wifi_usable_channel_filter|. Indicates if the channel list should
     *        be filtered based on additional criteria. If filter_mask is not
     *        specified, driver should return list of usable channels purely
     *        based on regulatory constraints.
     * @param max_size maximum number of |wifi_usable_channel|
     * @param size actual number of |wifi_usable_channel| entries returned by driver
     * @param channels list of usable channels represented by |wifi_usable_channel|
     */
    wifi_error (*wifi_get_usable_channels)(wifi_handle handle, u32 band_mask, u32 iface_mode_mask,
                                           u32 filter_mask, u32 max_size, u32* size,
                                           wifi_usable_channel* channels);

    /**
     * Trigger wifi subsystem restart to reload firmware
     */
    wifi_error (*wifi_trigger_subsystem_restart)(wifi_handle handle);

    /**
     * Invoked to set that the device is operating in an indoor environment.
     * @param handle global wifi_handle
     * @param isIndoor: true if the device is operating in an indoor
     *        environment, false otherwise.
     * @return Synchronous wifi_error
     */
    wifi_error (*wifi_set_indoor_state)(wifi_handle handle, bool isIndoor);

    /**@brief wifi_get_supported_radio_combinations_matrix
     *        Request all the possible radio combinations this device can offer.
     * @param handle global wifi_handle
     * @param max_size maximum size allocated for filling the wifi_radio_combination_matrix
     * @param wifi_radio_combination_matrix to return all the possible radio
     *        combinations.
     * @param size actual size of wifi_radio_combination_matrix returned from
     *        lower layer
     *
     */
    wifi_error (*wifi_get_supported_radio_combinations_matrix)(
            wifi_handle handle, u32 max_size, u32* size,
            wifi_radio_combination_matrix* radio_combination_matrix);

    /**@brief wifi_nan_rtt_chre_enable_request
     *        Request to enable CHRE NAN RTT
     * @param transaction_id: NAN transaction id
     * @param wifi_interface_handle
     * @param NanEnableRequest request message
     * @return Synchronous wifi_error
     */
    wifi_error (*wifi_nan_rtt_chre_enable_request)(transaction_id id, wifi_interface_handle iface,
                                                   NanEnableRequest* msg);

    /**@brief wifi_nan_rtt_chre_disable_request
     *        Request to disable CHRE NAN RTT
     * @param transaction_id: NAN transaction id
     * @param wifi_interface_handle
     * @return Synchronous wifi_error
     */
    wifi_error (*wifi_nan_rtt_chre_disable_request)(transaction_id id, wifi_interface_handle iface);

    /**@brief wifi_chre_register_handler
     *        register a handler to get the state of CHR
     * @param wifi_interface_handle
     * @param wifi_chre_handler: callback function pointer
     * @return Synchronous wifi_error
     */
    wifi_error (*wifi_chre_register_handler)(wifi_interface_handle iface,
                                             wifi_chre_handler handler);

    /**@brief wifi_enable_tx_power_limits
     *        Enable WiFi Tx power limis
     * @param wifi_interface_handle
     * @param isEnable : If enable TX limit or not
     * @return Synchronous wifi_error
     */
    wifi_error (*wifi_enable_tx_power_limits)(wifi_interface_handle iface, bool isEnable);

    /**@brief wifi_get_cached_scan_results
     *        Retrieve scan results cached in wifi firmware
     * @param wifi_interface_handle
     * @param wifi_cached_scan_result_handler : callback function pointer
     * @return Synchronous wifi_error
     */
    wifi_error (*wifi_get_cached_scan_results)(wifi_interface_handle iface,
                                               wifi_cached_scan_result_handler handler);
    /*
     * when adding new functions make sure to add stubs in
     * hal_tool.cpp::init_wifi_stub_hal_func_table
     */
} wifi_hal_fn;

wifi_error init_wifi_vendor_hal_func_table(wifi_hal_fn* fn);
typedef wifi_error (*init_wifi_vendor_hal_func_table_t)(wifi_hal_fn* fn);

#ifdef __cplusplus
}
#endif

#endif
