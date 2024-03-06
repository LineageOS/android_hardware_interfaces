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
 * Used to enumerate the various level of automation that can be expressed by the
 * VEHICLE_DRIVING_AUTOMATION_CURRENT_LEVEL property.
 */
@VintfStability
@Backing(type="int")
enum VehicleAutonomousState {
    /**
     * No automation. ADAS systems are limited to providing warnings and momentary assistance. The
     * driver is in constant supervision of all driving tasks and must steer, brake or accelerate as
     * needed to maintain safety, and is still responsible for driving while the ADAS systems are
     * engaged. Usage should be in accordance to Level 0 definition in J3016_202104 version of
     * vehicle autonomy levels defined by SAE.
     */
    LEVEL_0 = 0,
    /**
     * Driver assistance. ADAS systems can provide steering or brake/acceleration support to the
     * driver. The driver is in constant supervision of all driving tasks and must steer, brake or
     * accelerate as needed to maintain safety, and is still responsible for driving while the ADAS
     * systems are engaged. Usage should be in accordance to Level 1 definition in J3016_202104
     * version of vehicle autonomy levels defined by SAE.
     */
    LEVEL_1 = 1,
    /**
     * Partial automation. ADAS systems can provide both steering and brake/acceleration support to
     * the driver at the same time. The driver is in constant supervision of all driving tasks and
     * must steer, brake or accelerate as needed to maintain safety, and is still responsible for
     * driving while the ADAS systems are engaged. Usage should be in accordance to Level 2
     * definition in J3016_202104 version of vehicle autonomy levels defined by SAE.
     */
    LEVEL_2 = 2,
    /**
     * Conditional automation. ADAS systems can drive the vehicle under limited conditions and will
     * not operate unless all required conditions are met. The driver is required to take over
     * control of the vehicle when requested to do so by the ADAS systems, however is not
     * responsible for driving while the ADAS systems are engaged. Usage should be in accordance to
     * Level 3 definition in J3016_202104 version of vehicle autonomy levels defined by SAE.
     */
    LEVEL_3 = 3,
    /**
     * High automation. ADAS systems can drive the vehicle under limited conditions and will not
     * operate unless all required conditions are met. The driver is not required to take over
     * control of the vehicle and is not responsible for driving while the ADAS systems are engaged.
     * Usage should be in accordance to Level 4 definition in J3016_202104 version of vehicle
     * autonomy levels defined by SAE.
     */
    LEVEL_4 = 4,
    /**
     * Full automation. ADAS systems can drive the vehicle under all conditions. The driver is not
     * required to take over control of the vehicle and is not responsible for driving while the
     * ADAS systems are engaged. Usage should be in accordance to Level 5 definition in J3016_202104
     * version of vehicle autonomy levels defined by SAE.
     */
    LEVEL_5 = 5,
}
