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
 * Used to enumerate the current state of VehicleProperty#TRAILER_PRESENT.
 */
@VintfStability
@Backing(type="int")
enum TrailerState {
    /**
     * This state is used as an alternative for any TrailerState value that is not defined in the
     * platform. Ideally, implementations of VehicleProperty#TRAILER_PRESENT should not use this
     * state. The framework can use this field to remain backwards compatible if TrailerState is
     * extended to include additional states.
     */
    UNKNOWN = 0,
    /**
     * A trailer is not attached to the vehicle.
     */
    NOT_PRESENT = 1,
    /**
     * A trailer is attached to the vehicle.
     */
    PRESENT = 2,
    /**
     * The state of the trailer is not available due to an error.
     */
    ERROR = 3,
}
