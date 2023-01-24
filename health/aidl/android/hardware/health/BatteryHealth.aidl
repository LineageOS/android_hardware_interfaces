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

package android.hardware.health;

/**
 * Possible values for Battery Health.
 * Note: These are currently in sync with BatteryManager and must not
 * be extended / altered.
 */
@VintfStability
@Backing(type="int")
enum BatteryHealth {
    /**
     * Battery health is not supported from the device.
     */
    UNKNOWN = 1,
    GOOD = 2,
    /**
     * Must be consistent with BatteryChargingState.
     * If BatteryHealth is OVERHEAT, then BatteryChargingState must be TOO_HOT.
     */
    OVERHEAT = 3,
    DEAD = 4,
    OVER_VOLTAGE = 5,
    /**
     * Battery experienced an unknown/unspecified failure.
     */
    UNSPECIFIED_FAILURE = 6,
    /**
     * Must be consistent with BatteryChargingState.
     * If BatteryHealth is COLD, then BatteryChargingState must be TOO_COLD.
     */
    COLD = 7,
    /**
     * Battery health is marginal.
     */
    FAIR = 8,
    /**
     * The reserve data below 10 are used to recognize the battery real health.
     */
    /**
     * There is not enough information to determine an accurate
     * value. The value might become UNSPECIFIED_FAILURE, DEAD
     * or any other state except for UNKNOWN later.
     */
    NOT_AVAILABLE = 11,
    /**
     * The internal data is inconsistent and the battery needs to
     * go through a recalibration process. The value might become
     * UNSPECIFIED_FAILURE, DEAD or any other state except for UNKNOWN later.
     */
    INCONSISTENT = 12,
}
