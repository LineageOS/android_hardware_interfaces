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

package android.hardware.wifi;

/**
 * Ranging status.
 */
@VintfStability
@Backing(type="int")
enum RttStatus {
    SUCCESS = 0,
    /**
     * General failure status.
     */
    FAILURE = 1,
    /**
     * Target STA does not respond to request.
     */
    FAIL_NO_RSP = 2,
    /**
     * Request rejected. Applies to 2-sided RTT only.
     */
    FAIL_REJECTED = 3,
    FAIL_NOT_SCHEDULED_YET = 4,
    /**
     * Timing measurement times out.
     */
    FAIL_TM_TIMEOUT = 5,
    /**
     * Target on different channel, cannot range.
     */
    FAIL_AP_ON_DIFF_CHANNEL = 6,
    /**
     * Ranging not supported.
     */
    FAIL_NO_CAPABILITY = 7,
    /**
     * Request aborted for an unknown reason.
     */
    ABORTED = 8,
    /**
     * Invalid T1-T4 timestamp.
     */
    FAIL_INVALID_TS = 9,
    /**
     * 11mc protocol failed.
     */
    FAIL_PROTOCOL = 10,
    /**
     * Request could not be scheduled.
     */
    FAIL_SCHEDULE = 11,
    /**
     * Responder cannot collaborate at time of request.
     */
    FAIL_BUSY_TRY_LATER = 12,
    /**
     * Bad request args.
     */
    INVALID_REQ = 13,
    /**
     * WiFi not enabled.
     */
    NO_WIFI = 14,
    /**
     * Responder overrides param info, cannot range with new params.
     */
    FAIL_FTM_PARAM_OVERRIDE = 15,
    /**
     * NAN ranging negotiation failure.
     */
    NAN_RANGING_PROTOCOL_FAILURE = 16,
    /**
     * NAN concurrency not supported (NDP + RTT).
     */
    NAN_RANGING_CONCURRENCY_NOT_SUPPORTED = 17,
}
