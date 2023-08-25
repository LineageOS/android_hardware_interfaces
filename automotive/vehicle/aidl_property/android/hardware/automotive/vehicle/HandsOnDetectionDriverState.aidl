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
 * Used to enumerate the current driver state of Hands On Detection (HOD).
 */
@VintfStability
@Backing(type="int")
enum HandsOnDetectionDriverState {
    /**
     * This state is used as an alternative for any HandsOnDetectionDriverState value that is not
     * defined in the platform. Ideally, implementations of
     * VehicleProperty#HANDS_ON_DETECTION_DRIVER_STATE should not use this state. The framework
     * can use this field to remain backwards compatible if HandsOnDetectionDriverState is
     * extended to include additional states.
     */
    OTHER = 0,
    /**
     * The system detects that the driver has their hands on the steering wheel.
     */
    HANDS_ON = 1,
    /**
     * The system detects that the driver has their hands off the steering wheel.
     */
    HANDS_OFF = 2,
}
