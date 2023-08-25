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

/**
 * An availability state is sent to the subscribers in response to a change in the available
 * layers as part of a VmsMessageType.AVAILABILITY_CHANGE message, or in response to a
 * VmsMessageType.AVAILABILITY_REQUEST message as part of a VmsMessageType.AVAILABILITY_RESPONSE.
 * The VMS service issues monotonically increasing sequence numbers, and in case a subscriber
 * receives a smaller sequence number, it should ignore the message. An available associated layer
 * is a layer with a list of publisher IDs:
 * - Layer type
 * - Layer subtype
 * - Layer version
 * - Number of publisher IDs (N)
 * - N x publisher ID
 */
@VintfStability
@Backing(type="int")
enum VmsAvailabilityStateIntegerValuesIndex {
    /*
     * The message type as enumerated by VmsMessageType enum.
     */
    MESSAGE_TYPE = 0,
    SEQUENCE_NUMBER = 1,
    NUMBER_OF_ASSOCIATED_LAYERS = 2,
    LAYERS_START = 3,
}
