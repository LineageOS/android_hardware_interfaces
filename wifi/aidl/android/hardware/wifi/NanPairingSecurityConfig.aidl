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

import android.hardware.wifi.NanCipherSuiteType;
import android.hardware.wifi.NanPairingAkm;
import android.hardware.wifi.NanPairingSecurityType;

/**
 * Configuration of NAN data-path security.
 */
@VintfStability
parcelable NanPairingSecurityConfig {
    /**
     * Security configuration of the NAN pairing. |NanPairingSecurityType.PMK| for verification.
     * |NanPairingSecurityType.PASSPHRASE| and |NanPairingSecurityType.OPPORTUNISTIC| for setup
     */
    NanPairingSecurityType securityType;
    /**
     * Optional Pairwise Master Key (PMK). Must be specified (and is only used) if |securityType| is
     * set to |NanDataPathSecurityType.PMK|.
     * Ref: IEEE 802.11i
     */
    byte[32] pmk;
    /**
     * Optional Passphrase. Must be specified (and is only used) if |securityType| is set to
     * |NanDataPathSecurityType.PASSPHRASE|.
     * Min length: |IWifiNanIface.MIN_DATA_PATH_CONFIG_PASSPHRASE_LENGTH|
     * Max length: |IWifiNanIface.MAX_DATA_PATH_CONFIG_PASSPHRASE_LENGTH|
     * NAN Spec: Appendix: Mapping passphrase to PMK for NCS-SK Cipher Suites
     */
    byte[] passphrase;
    /**
     * The AKM for key exchange
     */
    NanPairingAkm akm;
    /**
     * Cipher type for pairing. Must be one of |NanCipherSuiteType.PUBLIC_KEY_PASN_128_MASK|
     * or |NanCipherSuiteType.PUBLIC_KEY_PASN_256_MASK|.
     */
    NanCipherSuiteType cipherType;
}
