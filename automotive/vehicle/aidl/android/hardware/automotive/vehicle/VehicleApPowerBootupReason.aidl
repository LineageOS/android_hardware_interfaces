/*
 * Copyright (C) 2022 The Android Open Source Project
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
 * Vehicle AP power bootup reason.
 */
@VintfStability
@Backing(type="int")
enum VehicleApPowerBootupReason {
    /**
     * Power on due to user's pressing of power key or rotating of ignition
     * switch.
     */
    USER_POWER_ON = 0,
    /**
     * Automatic power on triggered by door unlock or any other kind of automatic
     * user detection.
     */
    SYSTEM_USER_DETECTION = 1,
    /**
     * Automatic power on to execute a remote task. This is triggered by
     * receiving a wakeup message from TCU wakeup client.
     */
    SYSTEM_REMOTE_ACCESS = 2,
}
