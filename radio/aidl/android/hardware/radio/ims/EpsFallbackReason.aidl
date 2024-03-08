/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
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

package android.hardware.radio.ims;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
@Backing(type="int")
enum EpsFallbackReason {
    /**
     * If VoNR is not supported and EPS fallback is not triggered by network then UE initiated EPS
     * fallback would be triggered by IMS stack with this reason. The modem shall locally release
     * the 5G NR SA RRC connection and acquire the LTE network and perform a tracking area update
     * procedure. After the EPS fallback procedure is completed, the call setup for voice will
     * be established.
     */
    NO_NETWORK_TRIGGER = 1,

    /**
     * If the UE doesn't receive any response for SIP INVITE within a certain time in 5G NR SA,
     * UE initiated EPS fallback will be triggered with this reason. The modem shall reset its data
     * buffer of IMS PDUs to prevent the ghost call. After the EPS fallback procedure is completed,
     * the VoLTE call will be established.
     */
    NO_NETWORK_RESPONSE = 2,
}
