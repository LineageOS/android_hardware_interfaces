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
 * WPS Configuration Error.
 */
@VintfStability
@Backing(type="int")
enum WpsConfigError {
    NO_ERROR = 0,
    OOB_IFACE_READ_ERROR = 1,
    DECRYPTION_CRC_FAILURE = 2,
    CHAN_24_NOT_SUPPORTED = 3,
    CHAN_50_NOT_SUPPORTED = 4,
    SIGNAL_TOO_WEAK = 5,
    NETWORK_AUTH_FAILURE = 6,
    NETWORK_ASSOC_FAILURE = 7,
    NO_DHCP_RESPONSE = 8,
    FAILED_DHCP_CONFIG = 9,
    IP_ADDR_CONFLICT = 10,
    NO_CONN_TO_REGISTRAR = 11,
    MULTIPLE_PBC_DETECTED = 12,
    ROGUE_SUSPECTED = 13,
    DEVICE_BUSY = 14,
    SETUP_LOCKED = 15,
    MSG_TIMEOUT = 16,
    REG_SESS_TIMEOUT = 17,
    DEV_PASSWORD_AUTH_FAILURE = 18,
    CHAN_60G_NOT_SUPPORTED = 19,
    PUBLIC_KEY_HASH_MISMATCH = 20,
}
