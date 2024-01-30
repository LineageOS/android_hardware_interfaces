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

package android.hardware.radio.ims;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable ConnectionFailureInfo {
    @VintfStability
    @Backing(type="int")
    enum ConnectionFailureReason {
        /** Access class check failed */
        REASON_ACCESS_DENIED = 1,
        /** 3GPP Non-access stratum failure */
        REASON_NAS_FAILURE = 2,
        /** Random access failure */
        REASON_RACH_FAILURE = 3,
        /** Radio link failure */
        REASON_RLC_FAILURE = 4,
        /** Radio connection establishment rejected by network */
        REASON_RRC_REJECT = 5,
        /** Radio connection establishment timed out */
        REASON_RRC_TIMEOUT = 6,
        /** Device currently not in service */
        REASON_NO_SERVICE = 7,
        /** The PDN is no more active */
        REASON_PDN_NOT_AVAILABLE = 8,
        /** Radio resource is busy with another subscription */
        REASON_RF_BUSY = 9,
        REASON_UNSPECIFIED = 0xFFFF,
    }

    /**
     * Values are REASON_* constants
     */
    ConnectionFailureReason failureReason;

    /**
     * Failure cause code from network or modem specific to the failure
     */
    int causeCode;

    /**
     * Retry wait time provided by network in milliseconds
     */
    int waitTimeMillis;
}
