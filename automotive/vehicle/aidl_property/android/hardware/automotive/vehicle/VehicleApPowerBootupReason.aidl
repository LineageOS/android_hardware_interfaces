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
     * receiving a wakeup message from an external system in the vehicle.
     */
    SYSTEM_REMOTE_ACCESS = 2,
    /**
     * Automatic power on to enter garage mode. This is triggered by
     * receiving a wakeup message from an external system in the vehicle.
     *
     * Note that this does not necessarily mean Android will enter
     * the garage mode since user may enter the vehicle after this is set.
     * The system will only enter garage mode if VEHICLE_IN_USE is not true
     * upon check.
     *
     * To consider the Time-Of-Check-Time-Of-Use issue, there is a slight chance
     * that the vehicle become in-use after car service does the VEHICLE_IN_USE
     * check. The external power controller must also check whether the vehicle
     * is in use upon receiving the SHUTDOWN_REQUEST, before sending out
     * SHUTDOWN_PREPARE, to make sure the system does not enter garage mode or
     * shutdown if the vehicle is currently in use.
     */
    SYSTEM_ENTER_GARAGE_MODE = 3,
}
