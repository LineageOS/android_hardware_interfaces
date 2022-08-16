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

import android.hardware.automotive.vehicle.VmsMessageWithLayerIntegerValuesIndex;

/*
 * A VMS message with a layer and publisher ID is sent as part of a
 * VmsMessageType.SUBSCRIBE_TO_PUBLISHER, VmsMessageType.UNSUBSCRIBE_TO_PUBLISHER messages and
 * VmsMessageType.DATA .
 */
@VintfStability
@Backing(type="int")
enum VmsMessageWithLayerAndPublisherIdIntegerValuesIndex {
    /*
     * The message type as enumerated by VmsMessageType enum.
     */
    MESSAGE_TYPE = 0,
    LAYER_TYPE = 1,
    LAYER_SUBTYPE = 2,
    LAYER_VERSION = 3,
    PUBLISHER_ID = 4,
}
