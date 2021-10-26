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
 * Possble mask of values for GroupMgmtCipher param.
 * See /external/wpa_supplicant_8/src/common/defs.h for
 * all possible values (starting at WPA_CIPHER_BIP_GMAC_128).
 */
@VintfStability
@Backing(type="int")
enum GroupMgmtCipherMask {
    /**
     * BIP_GMAC-128 Group Management Cipher
     */
    BIP_GMAC_128 = 1 << 11,
    /**
     * BIP_GMAC-256 Group Management Cipher
     */
    BIP_GMAC_256 = 1 << 12,
    /**
     * BIP_CMAC-256 Group Management Cipher
     */
    BIP_CMAC_256 = 1 << 13,
}
