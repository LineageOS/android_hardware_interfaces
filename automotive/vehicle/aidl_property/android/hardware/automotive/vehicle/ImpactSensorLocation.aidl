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
 * Used to enumerate the various impact sensor locations on the car.
 */
@VintfStability
@Backing(type="int")
enum ImpactSensorLocation {
    /**
     * Other impact sensor location. Ideally this should never be used.
     */
    OTHER = 0x01,
    /**
     * Frontal impact sensor. Used for the sensor that detects head-on impact.
     */
    FRONT = 0x02,
    /**
     * Front-left door side impact sensor. Used for the sensor that detects collisions from the
     * side, in particular on the front-left door.
     */
    FRONT_LEFT_DOOR_SIDE = 0x04,
    /**
     * Front-right door side impact sensor. Used for the sensor that detects collisions from the
     * side, in particular on the front-right door.
     */
    FRONT_RIGHT_DOOR_SIDE = 0x08,
    /**
     * Rear-left door side impact sensor. Used for the sensor that detects collisions from the
     * side, in particular on the rear-left door.
     */
    REAR_LEFT_DOOR_SIDE = 0x10,
    /**
     * Rear-right door side impact sensor. Used for the sensor that detects collisions from the
     * side, in particular on the rear-right door.
     */
    REAR_RIGHT_DOOR_SIDE = 0x20,
    /**
     * Rear impact sensor. Used for the sensor that detects collisions from the rear.
     */
    REAR = 0x40,
}
