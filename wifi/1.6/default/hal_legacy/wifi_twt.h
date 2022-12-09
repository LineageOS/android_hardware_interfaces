/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef __WIFI_HAL_TWT_H__
#define __WIFI_HAL_TWT_H__

#include "wifi_hal.h"

typedef struct {
    u8 requester_supported;      // 0 for not supporting requester
    u8 responder_supported;      // 0 for not supporting responder
    u8 broadcast_twt_supported;  // 0 for not supporting broadcast TWT
    u8 flexibile_twt_supported;  // 0 for not supporting flexible TWT
} TwtCapability;

typedef struct {
    TwtCapability device_capability;
    TwtCapability peer_capability;
} TwtCapabilitySet;

// For all optional fields below, if no value specify -1
typedef struct {
    u8 config_id;          // An unique ID for an individual TWT request
    u8 negotiation_type;   // 0 for individual TWT, 1 for broadcast TWT
    u8 trigger_type;       // 0 for non-triggered TWT, 1 for triggered TWT
    s32 wake_dur_us;       // Proposed wake duration in us
    s32 wake_int_us;       // Average wake interval in us
    s32 wake_int_min_us;   // Min wake interval in us. Optional.
    s32 wake_int_max_us;   // Max wake interval in us. Optional.
    s32 wake_dur_min_us;   // Min wake duration in us. Optional.
    s32 wake_dur_max_us;   // Max wake duration in us. Optional.
    s32 avg_pkt_size;      // Average bytes of each packet to send in each wake
                           // duration. Optional.
    s32 avg_pkt_num;       // Average number of packets to send in each wake
                           // duration. Optional.
    s32 wake_time_off_us;  // First wake duration time offset in us. Optional.
} TwtSetupRequest;

typedef enum {
    TWT_SETUP_SUCCESS = 0,  // TWT setup is accepted.
    TWT_SETUP_REJECT = 1,   // TWT setup is rejected by AP.
    TWT_SETUP_TIMEOUT = 2,  // TWT setup response from AP times out.
    TWT_SETUP_IE = 3,       // AP sent TWT Setup IE parsing failure.
    TWT_SETUP_PARAMS = 4,   // AP sent TWT Setup IE Parameters invalid.
    TWT_SETUP_ERROR = 255,  // Generic error
} TwtSetupReasonCode;

typedef struct {
    u8 config_id;  // An unique ID for an individual TWT request
    u8 status;     // 0 for success, non-zero for failure
    TwtSetupReasonCode reason_code;
    u8 negotiation_type;   // 0 for individual TWT, 1 for broadcast TWT
    u8 trigger_type;       // 0 for non-triggered TWT, 1 for triggered TWT
    s32 wake_dur_us;       // Proposed wake duration in us
    s32 wake_int_us;       // Average wake interval in us
    s32 wake_time_off_us;  // First wake duration time offset in us.
} TwtSetupResponse;

typedef struct {
    u8 config_id;         // An unique ID for an individual TWT request
    u8 all_twt;           // 0 for individual setp request, 1 for all TWT
    u8 negotiation_type;  // 0 for individual TWT, 1 for broadcast TWT
} TwtTeardownRequest;

typedef enum {
    TWT_TD_RC_HOST = 0,        // Teardown triggered by Host
    TWT_TD_RC_PEER = 1,        // Peer initiated teardown
    TWT_TD_RC_MCHAN = 2,       // Teardown due to MCHAN Active
    TWT_TD_RC_MCNX = 3,        // Teardown due to MultiConnection
    TWT_TD_RC_CSA = 4,         // Teardown due to CSA
    TWT_TD_RC_BTCX = 5,        // Teardown due to BT Coex
    TWT_TD_RC_SETUP_FAIL = 6,  // Setup fails midway. Teardown all connections
    TWT_TD_RC_SCHED = 7,       // Teardown by TWT Scheduler
    TWT_TD_RC_ERROR = 255,     // Generic error cases
} TwtTeardownReason;

typedef struct {
    u8 config_id;  // An unique ID for an individual TWT request
    u8 all_twt;    // 0 for individual setp request, 1 for all TWT
    u8 status;     // 0 for success, non-zero for failure
    TwtTeardownReason reason;
} TwtTeardownCompletion;

typedef struct {
    u8 config_id;        // An unique ID for an individual TWT request
    u8 all_twt;          // 0 for individual setup request, 1 for all TWT
    s32 resume_time_us;  // If -1, TWT is suspended for indefinite time.
                         // Otherwise, TWT is suspended for resume_time_us
} TwtInfoFrameRequest;

typedef enum {
    TWT_INFO_RC_HOST = 0,   // Host initiated TWT Info frame */
    TWT_INFO_RC_PEER = 1,   // Peer initiated TWT Info frame
    TWT_INFO_RC_ERROR = 2,  // Generic error conditions */
} TwtInfoFrameReason;

// TWT Info frame triggered externally.
// Device should not send TwtInfoFrameReceived to Host for internally
// triggered TWT Info frame during SCAN, MCHAN operations.
typedef struct {
    u8 config_id;  // An unique ID for an individual TWT request
    u8 all_twt;    // 0 for individual setup request, 1 for all TWT
    u8 status;     // 0 for success, non-zero for failure
    TwtInfoFrameReason reason;
    u8 twt_resumed;  // 1 - TWT resumed, 0 - TWT suspended
} TwtInfoFrameReceived;

typedef struct {
    u8 config_id;
    u32 avg_pkt_num_tx;   // Average number of Tx packets in each wake duration.
    u32 avg_pkt_num_rx;   // Average number of Rx packets in each wake duration.
    u32 avg_tx_pkt_size;  // Average bytes per Rx packet in each wake duration.
    u32 avg_rx_pkt_size;  // Average bytes per Rx packet in each wake duration.
    u32 avg_eosp_dur_us;  // Average duration of early terminated SP
    u32 eosp_count;       // Count of early terminations
    u32 num_sp;           // Count of service period (SP), also known as wake duration.
} TwtStats;

// Asynchronous notification from the device.
// For example, TWT was torn down by the device and later when the device is
// ready, it can send this async notification.
// This can be expandable in future.
typedef enum {
    TWT_NOTIF_ALLOW_TWT = 1,  // Device ready to process TWT Setup request
} TwtNotification;

typedef struct {
    TwtNotification notification;
} TwtDeviceNotify;

// Callbacks for various TWT responses and events
typedef struct {
    // Callback for TWT setup response
    void (*EventTwtSetupResponse)(TwtSetupResponse* event);
    // Callback for TWT teardown completion
    void (*EventTwtTeardownCompletion)(TwtTeardownCompletion* event);
    // Callback for TWT info frame received event
    void (*EventTwtInfoFrameReceived)(TwtInfoFrameReceived* event);
    // Callback for TWT notification from the device
    void (*EventTwtDeviceNotify)(TwtDeviceNotify* event);
} TwtCallbackHandler;

#endif /* __WIFI_HAL_TWT_H__ */
