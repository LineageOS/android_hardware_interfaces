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
 * Publishers send the VMS service their information and assigned in response a publisher ID.
 * Subscribers can request the publisher information for a publisher ID they received in other
 * messages.
 */
@VintfStability
@Backing(type="int")
enum VmsPublisherInformationIntegerValuesIndex {
    /*
     * The message type as enumerated by VmsMessageType enum.
     */
    MESSAGE_TYPE = 0,
    PUBLISHER_ID = 1,
}
