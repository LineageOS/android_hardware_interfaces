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

package android.hardware.wifi.supplicant;

/**
 * DppStatusErrorCode: The possible values for the DPP status and error codes
 * in the DPP protocol.
 * See Easy Connect specification V2.0 section 8.3.4 for details.
 */
@VintfStability
@Backing(type="int")
enum DppStatusErrorCode {
    UNKNOWN = -1,
    SUCCESS = 0,
    NOT_COMPATIBLE = 1,
    AUTH_FAILURE = 2,
    UNWRAP_FAILURE = 3,
    BAD_GROUP = 4,
    CONFIGURE_FAILURE = 5,
    RESPONSE_PENDING = 6,
    INVALID_CONNECTOR = 7,
    NO_MATCH = 8,
    CONFIG_REJECTED = 9,
    NO_AP = 10,
    CONFIGURE_PENDING = 11,
    CSR_NEEDED = 12,
    CSR_BAD = 13,
    NEW_KEY_NEEDED = 14,
}
