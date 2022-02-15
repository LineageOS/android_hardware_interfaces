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
 * P2P group capability.
 * See /external/wpa_supplicant_8/src/common/ieee802_11_defs.h
 * for all possible values (starting at P2P_GROUP_CAPAB_GROUP_OWNER).
 */
@VintfStability
@Backing(type="int")
enum P2pGroupCapabilityMask {
    GROUP_OWNER = 1 << 0,
    PERSISTENT_GROUP = 1 << 1,
    GROUP_LIMIT = 1 << 2,
    INTRA_BSS_DIST = 1 << 3,
    CROSS_CONN = 1 << 4,
    PERSISTENT_RECONN = 1 << 5,
    GROUP_FORMATION = 1 << 6,
}
