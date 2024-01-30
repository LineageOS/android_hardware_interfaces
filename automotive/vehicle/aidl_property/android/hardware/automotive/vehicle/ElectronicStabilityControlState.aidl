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
 * Used to enumerate the state of Electronic Stability Control (ESC).
 */
@VintfStability
@Backing(type="int")
enum ElectronicStabilityControlState {

    /**
     * This state is used as an alternative to any ElectronicStabilityControlState value that is not
     * defined in the platform. Ideally, implementations of
     * VehicleProperty#ELECTRONIC_STABILITY_CONTROL_STATE should not use this state. The framework
     * can use this field to remain backwards compatible if ElectronicStabilityControlState is
     * extended to include additional states.
     */
    OTHER = 0,
    /**
     * ESC is enabled and monitoring safety, but is not actively controlling the tires to prevent
     * the car from skidding.
     */
    ENABLED = 1,
    /**
     * ESC is enabled and is actively controlling the tires to prevent the car from skidding.
     */
    ACTIVATED = 2,
}
