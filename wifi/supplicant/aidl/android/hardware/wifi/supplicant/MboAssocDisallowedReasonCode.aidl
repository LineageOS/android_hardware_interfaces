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
 *  MBO spec v1.2, 4.2.4 Table 14: MBO Association disallowed reason code attribute
 *  values.
 */
@VintfStability
@Backing(type="byte")
enum MboAssocDisallowedReasonCode {
    RESERVED = 0,
    UNSPECIFIED = 1,
    MAX_NUM_STA_ASSOCIATED = 2,
    AIR_INTERFACE_OVERLOADED = 3,
    AUTH_SERVER_OVERLOADED = 4,
    INSUFFICIENT_RSSI = 5,
}
