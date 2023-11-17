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
enum ConnectionEvent {
    // 2G GSM
    CS_SERVICE_GSM = 0,
    SIGNALLING_GSM = 1,

    // 2G GPRS packet services
    PS_SERVICE_GPRS = 2,
    SIGNALLING_GPRS = 3,

    // 3G packet services
    PS_SERVICE_3G = 4,
    SIGNALLING_3G = 5,

    // 4G LTE packet services
    NAS_SIGNALLING_LTE = 6,
    AS_SIGNALLING_LTE = 7,

    // VoLTE
    VOLTE_SIP = 8,
    VOLTE_RTP = 9,

    // 5G packet services
    NAS_SIGNALLING_5G = 10,
    AS_SIGNALLING_5G = 11,

    // VoNR
    VONR_SIP = 12,
    VONR_RTP = 13,
}
