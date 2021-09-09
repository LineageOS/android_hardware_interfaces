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

package android.hardware.radio;

import android.hardware.radio.KeepaliveType;

@VintfStability
parcelable KeepaliveRequest {
    /**
     * The format of the keepalive packet
     */
    KeepaliveType type;
    /**
     * Source address with type = family, in network byte order
     */
    byte[] sourceAddress;
    /**
     * Source port if relevant for the given type
     * INT_MAX: 0x7FFFFFFF denotes that the field is unused
     */
    int sourcePort;
    /**
     * Destination address with type = family, in network byte order
     */
    byte[] destinationAddress;
    /**
     * Destination if relevant for the given type
     * INT_MAX: 0x7FFFFFFF denotes that the field is unused
     */
    int destinationPort;
    /**
     * The max interval between packets, in milliseconds
     */
    int maxKeepaliveIntervalMillis;
    /**
     * Context ID, returned in setupDataCallResponse that uniquely identifies the data call to which
     * this keepalive must applied.
     */
    int cid;
}
