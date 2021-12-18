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

/**
 * Status codes for P2P operations.
 */
@VintfStability
@Backing(type="int")
enum P2pStatusCode {
    SUCCESS = 0,
    FAIL_INFO_CURRENTLY_UNAVAILABLE = 1,
    FAIL_INCOMPATIBLE_PARAMS = 2,
    FAIL_LIMIT_REACHED = 3,
    FAIL_INVALID_PARAMS = 4,
    FAIL_UNABLE_TO_ACCOMMODATE = 5,
    FAIL_PREV_PROTOCOL_ERROR = 6,
    FAIL_NO_COMMON_CHANNELS = 7,
    FAIL_UNKNOWN_GROUP = 8,
    FAIL_BOTH_GO_INTENT_15 = 9,
    FAIL_INCOMPATIBLE_PROV_METHOD = 10,
    FAIL_REJECTED_BY_USER = 11,
    SUCCESS_DEFERRED = 12,
}
