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
 * Used to enumerate the state of Blind Spot Warning State (BSW).
 */
@VintfStability
@Backing(type="int")
enum BlindSpotWarningState {

    /**
     * This state is used as an alternative to any BlindSpotWarningState value that is not
     * defined in the platform. Ideally, implementations of
     * VehicleProperty#BLIND_SPOT_WARNING_STATE should not use this state. The framework
     * can use this field to remain backwards compatible if BlindSpotWarningState is
     * extended to include additional states.
     */
    OTHER = 0,
    /**
     * BSW is enabled and monitoring safety, but no vehicle or object detected in the vehicle's
     * blind spot.
     */
    NO_WARNING = 1,
    /**
     * BSW is enabled, detects a vehicle or object in the vehicle's blind spot, and is actively
     * warning the user.
     */
    WARNING = 2,
}
