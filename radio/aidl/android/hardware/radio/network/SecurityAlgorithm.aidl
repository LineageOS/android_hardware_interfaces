/*
 * Copyright 2023 The Android Open Source Project
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

package android.hardware.radio.network;

/**
 * See IRadioNetwork.securityAlgorithmsUpdated for more details.
 *
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum SecurityAlgorithm {
    // GSM CS services (3GPP TS 43.020)
    A50 = 0,
    A51 = 1,
    A52 = 2,
    A53 = 3,
    A54 = 4,

    // GPRS PS services (3GPP TS 43.020)
    // These also refer to the respective integrity counterparts.
    // E.g. GEA1 = GIA1
    GEA0 = 14,
    GEA1 = 15,
    GEA2 = 16,
    GEA3 = 17,
    GEA4 = 18,
    GEA5 = 19,

    // 3G PS/CS services (3GPP TS 33.102)
    UEA0 = 29,
    UEA1 = 30,
    UEA2 = 31,

    // 4G PS services & 5G NSA (3GPP TS 33.401)
    EEA0 = 41,
    EEA1 = 42,
    EEA2 = 43,
    EEA3 = 44,

    // 5G PS services (3GPP TS 33.401 for 5G NSA and 3GPP TS 33.501 for 5G SA)
    NEA0 = 55,
    NEA1 = 56,
    NEA2 = 57,
    NEA3 = 58,

    // SIP layer security (See 3GPP TS 33.203)
    SIP_NULL = 68,
    AES_GCM = 69,
    AES_GMAC = 70,
    AES_CBC = 71,
    DES_EDE3_CBC = 72,
    AES_EDE3_CBC = 73,
    HMAC_SHA1_96 = 74,
    HMAC_SHA1_96_null = 75,
    HMAC_MD5_96 = 76,
    HMAC_MD5_96_null = 77,

    // RTP (see 3GPP TS 33.328)
    SRTP_AES_COUNTER = 87,
    SRTP_AES_F8 = 88,
    SRTP_HMAC_SHA1 = 89,

    // ePDG (3GPP TS 33.402)
    ENCR_AES_GCM_16 = 99,
    ENCR_AES_CBC = 100,
    AUTH_HMAC_SHA2_256_128 = 101,

    /** Unknown */
    UNKNOWN = 113,
    OTHER = 114,

    /** For proprietary algorithms */
    ORYX = 124,
}
