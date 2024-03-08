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

package android.hardware.radio.data;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable KeepaliveStatus {
    /**
     * Keepalive is currently active.
     */
    const int CODE_ACTIVE = 0;
    /**
     * Keepalive is inactive, which indicates an error.
     */
    const int CODE_INACTIVE = 1;
    /**
     * Requested keepalive has not yet been processed by the modem.
     * Only allowed in a RESPONSE message to a REQUEST.
     */
    const int CODE_PENDING = 2;

    /**
     * The sessionHandle provided by the API.
     */
    int sessionHandle;
    /**
     * Status for the given keepalive.
     * Values are CODE_
     */
    int code;
}
