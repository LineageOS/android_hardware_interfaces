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
 * Used to enumerate the current state of driver distraction monitoring.
 *
 * This enum could be extended in future releases to include additional feature states.
 */
@VintfStability
@Backing(type="int")
enum DriverDistractionState {
    /**
     * This state is used as an alternative for any DriverDistractionState value that is not
     * defined in the platform. Ideally, implementations of
     * VehicleProperty#DRIVER_DISTRACTION_STATE should not use this state. The framework
     * can use this field to remain backwards compatible if DriverDistractionState is
     * extended to include additional states.
     */
    OTHER = 0,
    /**
     * The system detects that the driver is attentive / not distracted.
     */
    NOT_DISTRACTED = 1,
    /**
     * The system detects that the driver is distracted, which can be anything that reduces the
     * driver's foucs on the primary task of driving/controlling the vehicle.
     */
    DISTRACTED = 2,
}
