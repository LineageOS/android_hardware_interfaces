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

package android.hardware.automotive.vehicle;

import android.hardware.automotive.vehicle.VmsBaseMessageIntegerValuesIndex;

/*
 * Handshake data sent as part of a VmsMessageType.START_SESSION message.
 *
 * A new session is initiated by sending a START_SESSION message with the
 * sender's identifier populated and the receiver's identifier set to -1.
 *
 * Identifier values are independently generated, but must be non-negative, and
 * increase monotonically between reboots.
 *
 * Upon receiving a START_SESSION with a mis-matching identifier, the receiver
 * must clear any cached VMS offering or subscription state and acknowledge the
 * new session by responding with a START_SESSION message that populates both
 * identifier fields.
 *
 * Any VMS messages received between initiation and completion of the handshake
 * must be discarded.
 */
@VintfStability
@Backing(type="int")
enum VmsStartSessionMessageIntegerValuesIndex {
    /*
     * The message type as enumerated by VmsMessageType enum.
     */
    MESSAGE_TYPE = 0,
    /*
     * Identifier field for the Android system service.
     */
    SERVICE_ID = 1,
    /*
     * Identifier field for the HAL client process.
     */
    CLIENT_ID = 2,
}
