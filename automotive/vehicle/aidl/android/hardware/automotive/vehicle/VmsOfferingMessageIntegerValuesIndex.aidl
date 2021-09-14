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
 * An offering can be sent by publishers as part of VmsMessageType.OFFERING in order to
 * advertise which layers they can publish and under which constraints: e.g., I can publish Layer X
 * if someone else will publish Layer Y.
 * The offering contains the publisher ID which was assigned to the publisher by the VMS service.
 * A single offering is represented as:
 * - Layer type
 * - Layer subtype
 * - Layer version
 * - Number of dependencies (N)
 * - N x (Layer type, Layer subtype, Layer version)
 */
@VintfStability
@Backing(type="int")
enum VmsOfferingMessageIntegerValuesIndex {
    /*
     * The message type as enumerated by VmsMessageType enum.
     */
    MESSAGE_TYPE = 0,
    PUBLISHER_ID = 1,
    NUMBER_OF_OFFERS = 2,
    OFFERING_START = 3,
}
