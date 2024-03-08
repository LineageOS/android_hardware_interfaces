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

package android.hardware.radio.network;

/**
 * Please note that registration state UNKNOWN is treated as "out of service" in Android telephony.
 * Registration state REG_DENIED must be returned if Location Update Reject (with cause 17 - Network
 * Failure) is received repeatedly from the network, to facilitate "managed roaming".
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum RegState {
    /**
     * Not registered, MT is not currently searching for a new operator to register
     */
    NOT_REG_MT_NOT_SEARCHING_OP = 0,
    /**
     * Registered, home network
     */
    REG_HOME = 1,
    /**
     * Not registered, but MT is currently searching for a new operator to register
     */
    NOT_REG_MT_SEARCHING_OP = 2,
    /**
     * Registration denied
     */
    REG_DENIED = 3,
    /**
     * Unknown
     */
    UNKNOWN = 4,
    /**
     * Registered, roaming
     */
    REG_ROAMING = 5,
    /**
     * Same as NOT_REG_MT_NOT_SEARCHING_OP but indicates that emergency calls are enabled
     */
    NOT_REG_MT_NOT_SEARCHING_OP_EM = 10,
    /**
     * Same as NOT_REG_MT_SEARCHING_OP but indicatees that emergency calls are enabled
     */
    NOT_REG_MT_SEARCHING_OP_EM = 12,
    /**
     * Same as REG_DENIED but indicates that emergency calls are enabled
     */
    REG_DENIED_EM = 13,
    /**
     * Same as UNKNOWN but indicates that emergency calls are enabled
     */
    UNKNOWN_EM = 14,
    /**
     * Emergency attached in EPS or in 5GS.
     * Reference: 3GPP TS 24.301 9.9.3.11 EPS attach type.
     * Reference: 3GPP TS 24.501 9.11.3.6 5GS registration result.
     */
    REG_EM = 20,
}
