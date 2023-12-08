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
    // 2G GSM circuit switched
    CS_SIGNALLING_GSM = 0,

    // 2G GPRS packet services
    PS_SIGNALLING_GPRS = 1,

    // 3G circuit switched
    CS_SIGNALLING_3G = 2,

    // 3G packet switched
    PS_SIGNALLING_3G = 3,

    // 4G LTE packet services
    NAS_SIGNALLING_LTE = 4,
    AS_SIGNALLING_LTE = 5,

    // VoLTE
    VOLTE_SIP = 6,
    VOLTE_RTP = 7,

    // 5G packet services
    NAS_SIGNALLING_5G = 8,
    AS_SIGNALLING_5G = 9,

    // VoNR
    VONR_SIP = 10,
    VONR_RTP = 11,
}
