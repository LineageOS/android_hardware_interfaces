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
enum VehicleApPowerStateShutdownParam {
    /**
     * AP must shutdown without Garage mode.
     * If AP need to shutdown as soon as possible, EMERGENCY_SHUTDOWN shall be used.
     */
    SHUTDOWN_IMMEDIATELY = 1,
    /**
     * AP can enter deep sleep instead of shutting down completely.
     * AP can postpone entering deep sleep to run Garage mode.
     */
    CAN_SLEEP = 2,
    /**
     * AP can only shutdown.
     * AP can postpone shutdown to run Garage mode.
     */
    SHUTDOWN_ONLY = 3,
    /**
     * AP can enter deep sleep, without Garage mode.
     * Depending on the actual implementation, it may shut down immediately
     */
    SLEEP_IMMEDIATELY = 4,
    /**
     * AP can hibernate (suspend to disk) without Garage mode.
     * Depending on the actual implementation, it may shut down immediately.
     */
    HIBERNATE_IMMEDIATELY = 5,
    /**
     * AP can enter hibernation (suspend to disk) instead of shutting down completely.
     * AP can postpone hibernation to run Garage mode.
     */
    CAN_HIBERNATE = 6,
    /**
     * AP must shutdown (gracefully) without a delay. AP cannot run Garage mode.
     * This type must be used only in critical situations when AP must shutdown as soon as possible.
     * CarService will only notify listeners, but will not wait for completion reports.
     */
    EMERGENCY_SHUTDOWN = 7,
}
