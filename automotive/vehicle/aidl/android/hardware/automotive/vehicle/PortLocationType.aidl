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

/**
 * Used by INFO_FUEL_DOOR_LOCATION/INFO_CHARGE_PORT_LOCATION to enumerate fuel door or
 * ev port location.
 */
@VintfStability
@Backing(type="int")
enum PortLocationType {
    /**
     * Default type if the vehicle does not know or report the Fuel door
     * and ev port location.
     */
    UNKNOWN = 0,
    FRONT_LEFT = 1,
    FRONT_RIGHT = 2,
    REAR_RIGHT = 3,
    REAR_LEFT = 4,
    FRONT = 5,
    REAR = 6,
}
