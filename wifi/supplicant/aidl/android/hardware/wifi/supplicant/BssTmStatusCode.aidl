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
 * IEEE Std 802.11-2016 - Table 9-357.
 * BTM status code filled in BSS transition management response frame.
 */
@VintfStability
@Backing(type="byte")
enum BssTmStatusCode {
    ACCEPT = 0,
    REJECT_UNSPECIFIED = 1,
    REJECT_INSUFFICIENT_BEACON = 2,
    REJECT_INSUFFICIENT_CAPABITY = 3,
    REJECT_BSS_TERMINATION_UNDESIRED = 4,
    REJECT_BSS_TERMINATION_DELAY_REQUEST = 5,
    REJECT_STA_CANDIDATE_LIST_PROVIDED = 6,
    REJECT_NO_SUITABLE_CANDIDATES = 7,
    REJECT_LEAVING_ESS = 8,
}
