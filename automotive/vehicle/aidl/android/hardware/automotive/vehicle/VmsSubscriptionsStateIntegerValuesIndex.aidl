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
 * A subscriptions state is sent to the publishers in response to a change in the subscriptions
 * as part of a VmsMessageType.SUBSCRIPTIONS_CHANGE, or in response to a
 * VmsMessageType.SUBSCRIPTIONS_REQUEST message as part of VmsMessageType.SUBSCRIPTIONS_RESPONSE.
 * The VMS service issues monotonically increasing sequence numbers, and in case a subscriber
 * receives a smaller sequence number it should ignore the message. The subscriptions are sent as a
 * list of layers followed by a list of associated layers: {Sequence number, N, M, N x layer, M x
 * associated layer} A subscribed layer is represented as three integers:
 * - Layer type
 * - Layer subtype
 * - Layer version
 * A subscribed associated layer is a layer with a list of publisher IDs. It is represented as:
 * - Layer type
 * - Layer subtype
 * - Layer version
 * - Number of publisher IDs (N)
 * - N x publisher ID
 */
@VintfStability
@Backing(type="int")
enum VmsSubscriptionsStateIntegerValuesIndex {
    /*
     * The message type as enumerated by VmsMessageType enum.
     */
    MESSAGE_TYPE = 0,
    SEQUENCE_NUMBER = 1,
    NUMBER_OF_LAYERS = 2,
    NUMBER_OF_ASSOCIATED_LAYERS = 3,
    SUBSCRIPTIONS_START = 4,
}
