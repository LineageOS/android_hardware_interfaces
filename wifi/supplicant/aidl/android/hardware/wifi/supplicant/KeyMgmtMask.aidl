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
 * Possible mask of values for KeyMgmt param.
 * See /external/wpa_supplicant_8/src/common/defs.h for
 * the historical values (starting at WPA_KEY_MGMT_IEEE8021X).
 */
@VintfStability
@Backing(type="int")
enum KeyMgmtMask {
    WPA_EAP = 1 << 0,
    WPA_PSK = 1 << 1,
    NONE = 1 << 2,
    IEEE8021X = 1 << 3,
    FT_EAP = 1 << 5,
    FT_PSK = 1 << 6,
    OSEN = 1 << 15,
    /**
     * WPA using EAP authentication with stronger SHA256-based algorithms
     */
    WPA_EAP_SHA256 = 1 << 7,
    /**
     * WPA pre-shared key with stronger SHA256-based algorithms
     */
    WPA_PSK_SHA256 = 1 << 8,
    /**
     * WPA3-Personal SAE Key management
     */
    SAE = 1 << 10,
    /**
     * WPA3-Enterprise Suite-B Key management
     */
    SUITE_B_192 = 1 << 17,
    /**
     * Enhacned Open (OWE) Key management
     */
    OWE = 1 << 22,
    /**
     * Easy Connect (DPP) Key management
     */
    DPP = 1 << 23,
    /*
     * WAPI Psk
     */
    WAPI_PSK = 1 << 12,
    /**
     * WAPI Cert
     */
    WAPI_CERT = 1 << 13,
    /**
     * FILS shared key authentication with sha-256
     */
    FILS_SHA256 = 1 << 18,
    /**
     * FILS shared key authentication with sha-384
     */
    FILS_SHA384 = 1 << 19,
    /**
     * Pre-Association Security Negotiation (PASN) Key management
     */
    PASN = 1 << 25,
}
