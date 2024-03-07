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

import android.hardware.radio.network.ConnectionEvent;
import android.hardware.radio.network.SecurityAlgorithm;

/**
 * A single occurrence capturing a notable change to previously reported
 * cryptography algorithms for a given network and network event.
 *
 * @hide
 */
@JavaDerive(toString=true)
@VintfStability
parcelable SecurityAlgorithmUpdate {
    /**
     * Type of connection event which is being reported on
     */
    ConnectionEvent connectionEvent;
    /**
     * Encryption algorithm which was used
     */
    SecurityAlgorithm encryption;
    /**
     * Integrity algorithm which was used
     */
    SecurityAlgorithm integrity;
    /**
     * Whether or not this connection event is associated with an
     * unauthenticated / unencrypted emergency session
     */
    boolean isUnprotectedEmergency;
}
