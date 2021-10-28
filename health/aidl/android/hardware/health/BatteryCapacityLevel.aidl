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
 * Battery capacity level. This enum provides additional information along side
 * with the battery capacity.
 * Clients of this HAL must use this value before inferring it from the
 * battery capacity.
 */
@VintfStability
@Backing(type="int")
enum BatteryCapacityLevel {
    /**
     * Battery capacity level is unsupported.
     * Battery capacity level must be set to this value if and only if the
     * implementation is unsupported.
     */
    UNSUPPORTED = -1,
    /**
     * Battery capacity level is unknown.
     * Battery capacity level must be set to this value if and only if battery
     * is not present or the battery capacity level is unknown/uninitialized.
     */
    UNKNOWN,
    /**
     * Battery is at critical level. The Android framework must schedule a
     * shutdown when it sees this value from the HAL.
     */
    CRITICAL,
    /**
     * Battery is low. The Android framework may limit the performance of
     * the device when it sees this value from the HAL.
     */
    LOW,
    /**
     * Battery level is normal.
     */
    NORMAL,
    /**
     * Battery level is high.
     */
    HIGH,
    /**
     * Battery is full. It must be set to FULL if and only if battery level is
     * 100.
     */
    FULL,
}
