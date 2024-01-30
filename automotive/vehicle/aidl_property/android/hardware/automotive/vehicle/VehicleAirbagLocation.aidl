/*
 * Copyright (C) 2023 The Android Open Source Project
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
 * Used to enumerate the various airbag locations per seat.
 */
@VintfStability
@Backing(type="int")
enum VehicleAirbagLocation {
    /**
     * This state is used as an alternative to any VehicleAirbagLocation value that is not defined
     * in the platform. Ideally, implementations of VehicleProperty::SEAT_AIRBAGS_DEPLOYED should
     * not use this state. The framework can use this field to remain backwards compatible if
     * VehicleAirbagLocation is extended to include additional states.
     */
    OTHER = 0x01,
    /**
     * Front airbags. This enum is for the airbags that protect the seated person from the front,
     * particularly the seated person's torso.
     */
    FRONT = 0x02,
    /**
     * Knee airbags. This enum is for the airbags that protect the seated person's knees.
     */
    KNEE = 0x04,
    /**
     * Left side airbags. This enum is for the side airbags that protect the left side of the seated
     * person.
     */
    LEFT_SIDE = 0x08,
    /**
     * Right side airbags. This enum is for the side airbags that protect the right side of the
     * seated person.
     */
    RIGHT_SIDE = 0x10,
    /**
     * Curtain airbags. This enum is for the airbags lined above the windows of the vehicle.
     */
    CURTAIN = 0x20,
}
