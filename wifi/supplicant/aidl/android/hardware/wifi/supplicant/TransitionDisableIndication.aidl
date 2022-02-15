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
 * WPA3™ Specification Addendum for WPA3 R3 - Table 3.
 * Transition Disable Indication filled in the third
 * 4-way handshake message.
 * See /external/wpa_supplicant_8/src/common/wpa_common.h for
 * all possible values (starting at TRANSITION_DISABLE_WPA3_PERSONAL).
 */
@VintfStability
@Backing(type="int")
enum TransitionDisableIndication {
    USE_WPA3_PERSONAL = 1 << 0,
    USE_SAE_PK = 1 << 1,
    USE_WPA3_ENTERPRISE = 1 << 2,
    USE_ENHANCED_OPEN = 1 << 3,
}
