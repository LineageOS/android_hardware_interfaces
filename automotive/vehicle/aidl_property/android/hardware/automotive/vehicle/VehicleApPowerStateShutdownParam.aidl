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
     * AP must shutdown immediately. Postponing is not allowed.
     */
    SHUTDOWN_IMMEDIATELY = 1,
    /**
     * AP can enter deep sleep instead of shutting down completely.
     */
    CAN_SLEEP = 2,
    /**
     * AP can only shutdown with postponing allowed.
     */
    SHUTDOWN_ONLY = 3,
    /**
     * AP may enter deep sleep, but must either sleep or shut down immediately.
     * Postponing is not allowed.
     */
    SLEEP_IMMEDIATELY = 4,
    /**
     * AP must hibernate (suspend to disk) immediately. Postponing is not allowed.
     * Depending on the actual implementation, it may shut down immediately
     */
    HIBERNATE_IMMEDIATELY = 5,
    /**
     * AP can enter hibernation (suspend to disk) instead of shutting down completely.
     */
    CAN_HIBERNATE = 6,
    /**
     * AP must shutdown (gracefully) without a delay.
     */
    EMERGENCY_SHUTDOWN = 7,
}
