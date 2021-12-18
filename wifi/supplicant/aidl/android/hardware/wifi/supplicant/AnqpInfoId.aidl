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
 * Access Network Query Protocol info ID elements
 * for IEEE Std 802.11u-2011.
 */
@VintfStability
@Backing(type="int")
enum AnqpInfoId {
    VENUE_NAME = 258,
    ROAMING_CONSORTIUM = 261,
    IP_ADDR_TYPE_AVAILABILITY = 262,
    NAI_REALM = 263,
    ANQP_3GPP_CELLULAR_NETWORK = 264,
    DOMAIN_NAME = 268,
}
