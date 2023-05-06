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

/**
 * Specifies the type of packet data protocol which is defined in TS 27.007 section 10.1.1.
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum PdpProtocolType {
    /**
     * Unknown protocol
     */
    UNKNOWN = -1,
    /**
     * Internet protocol
     */
    IP = 0,
    /**
     * Internet protocol, version 6
     */
    IPV6 = 1,
    /**
     * Virtual PDP type introduced to handle dual IP stack UE capability.
     */
    IPV4V6 = 2,
    /**
     * Point to point protocol
     */
    PPP = 3,
    /**
     * Transfer of Non-IP data to external packet data network
     */
    NON_IP = 4,
    /**
     * Transfer of Unstructured data to the Data Network via N6
     */
    UNSTRUCTURED = 5,
}
