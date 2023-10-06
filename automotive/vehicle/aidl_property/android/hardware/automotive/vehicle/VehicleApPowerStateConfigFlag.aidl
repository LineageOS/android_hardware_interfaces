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

@VintfStability
@Backing(type="int")
enum VehicleApPowerStateConfigFlag {
    /**
     * AP can enter deep sleep state. If not set, AP will shutdown from
     * VehicleApPowerState#SHUTDOWN_PREPARE power state when deep sleep is requested
     * (via VehicleApPowerStateShutdownParam#CAN_SLEEP or
     * VehicleApPowerStateShutdownParam#SLEEP_IMMEDIATELY flags)/
     */
    ENABLE_DEEP_SLEEP_FLAG = 0x1,
    /**
     * The power controller can power on AP from off state after timeout
     * specified in VehicleApPowerSet VEHICLE_AP_POWER_SET_SHUTDOWN_READY message.
     */
    CONFIG_SUPPORT_TIMER_POWER_ON_FLAG = 0x2,
    /**
     * AP can enter hibernation state. If not set, AP will shutdown from
     * VehicleApPowerState#SHUTDOWN_PREPARE when hibernation is requested
     * (via VehicleApPowerStateShutdownParam#CAN_HIBERNATE or
     *  VehicleApPowerStateShutdownParam#HIBERNATE_IMMEDIATELY flags)
     */
    ENABLE_HIBERNATION_FLAG = 0x4,
}
