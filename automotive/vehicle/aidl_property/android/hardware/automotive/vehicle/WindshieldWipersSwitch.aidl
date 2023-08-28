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
 * Used to enumerate the current position of VehicleProperty#WINDSHIELD_WIPERS_SWITCH.
 */
@VintfStability
@Backing(type="int")
enum WindshieldWipersSwitch {

    /**
     * This value is used as an alternative for any WindshieldWipersSwitch value that is not defined
     * in the platform. Ideally, implementations of VehicleProperty#WINDSHIELD_WIPERS_SWITCH should
     * not use this value. The framework can use this field to remain backwards compatible if
     * WindshieldWipersSwitch is extended to include additional values.
     */
    OTHER = 0,
    /**
     * The windshield wipers switch is set to the off position.
     */
    OFF = 1,
    /**
     * MIST mode performs a single wipe, and then returns to the OFF position.
     */
    MIST = 2,
    /**
     * INTERMITTENT_LEVEL_* modes performs intermittent wiping. As the level increases, the
     * intermittent time period decreases.
     */
    INTERMITTENT_LEVEL_1 = 3,
    INTERMITTENT_LEVEL_2 = 4,
    INTERMITTENT_LEVEL_3 = 5,
    INTERMITTENT_LEVEL_4 = 6,
    INTERMITTENT_LEVEL_5 = 7,
    /**
     * CONTINUOUS_LEVEL_* modes performs continuous wiping. As the level increases the speed of the
     * wiping increases as well.
     */
    CONTINUOUS_LEVEL_1 = 8,
    CONTINUOUS_LEVEL_2 = 9,
    CONTINUOUS_LEVEL_3 = 10,
    CONTINUOUS_LEVEL_4 = 11,
    CONTINUOUS_LEVEL_5 = 12,
    /**
     * AUTO allows the vehicle to decide the required wiping level based on the exterior weather
     * conditions.
     */
    AUTO = 13,
    /**
     * Windshield wipers are set to the service mode.
     */
    SERVICE = 14,
}
