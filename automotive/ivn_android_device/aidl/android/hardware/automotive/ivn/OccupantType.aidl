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

package android.hardware.automotive.ivn;

/**
 * Occupant type.
 *
 * This enum might be extended in the future.
 */
@VintfStability
@Backing(type="int")
enum OccupantType {
    /**
     * Represents the driver. There can be one or zero driver for the system. Zero driver situation
     * can happen if the system is configured to support only passengers.
     */
    DRIVER = 1,
    /**
     * Represents front passengers who sit in front side of car. Most cars will have only
     * one passenger of this type but this can be multiple.
     */
    FRONT_PASSENGER = 2,
    /** Represents passengers in rear seats. There can be multiple passengers of this type. */
    REAR_PASSENGER = 3,
}
