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
enum VehicleApPowerStateReport {
    /**
     * The device has booted. CarService has initialized and is ready to accept commands
     * from VHAL. The user is not logged in, and vendor apps and services are expected to
     * control the display and audio.
     * After reporting this state, AP will accept VehicleApPowerStateReq#ON or
     * VehicleApPowerStateReq#SHUTDOWN_PREPARE. Other power state requests are ignored.
     */
    WAIT_FOR_VHAL = 0x1,
    /**
     * AP is ready to suspend.
     * The AP will not send any more state reports after this.
     * After reporting this state, AP will accept VehicleApPowerStateReq#FINISHED.
     * Other power state requests are ignored.
     *
     * int32Values[1]: Time to turn AP back on, in seconds. Power controller should turn on
     *                 AP after the specified time has elapsed, so AP can run tasks like
     *                 update. If this value is 0, no wake up is requested. The power
     *                 controller may not necessarily support timed wake-up.
     */
    DEEP_SLEEP_ENTRY = 0x2,
    /**
     * AP is exiting from deep sleep state.
     * After reporting this state, AP will accept VehicleApPowerStateReq#ON or
     * VehicleApPowerStateReq#SHUTDOWN_PREPARE. Other power state requests are ignored.
     */
    DEEP_SLEEP_EXIT = 0x3,
    /**
     * AP sends this message repeatedly while cleanup and idle tasks execute.
     * After reporting this state, AP will accept VehicleApPowerStateReq#SHUTDOWN_PREPARE
     * requesting immediate shutdown or VehicleApPowerStateReq#CANCEL_SHUTDOWN. Other
     * power state requests are ignored.
     *
     * int32Values[1]: Time to postpone shutdown in ms. Maximum value is
     *                 5000 ms.
     *                 If AP needs more time, it will send another SHUTDOWN_POSTPONE
     *                 message before the previous one expires.
     */
    SHUTDOWN_POSTPONE = 0x4,
    /**
     * AP is ready to shutdown.
     * The AP will not send any more state reports after this.
     * After reporting this state, AP will accept VehicleApPowerStateReq#FINISHED.
     * Other power state requests are ignored.
     *
     * int32Values[1]: Time to turn AP back on, in seconds. Power controller should turn on
     *                 AP after the specified time has elapsed so AP can run tasks like
     *                 update. If this value is 0, no wake up is specified. The power
     *                 controller may not necessarily support timed wake-up.
     */
    SHUTDOWN_START = 0x5,
    /**
     * AP is entering its normal operating state.
     * After reporting this state, AP will accept VehicleApPowerStateReq#SHUTDOWN_PREPARE.
     * Other power state requests are ignored.
     */
    ON = 0x6,
    /**
     * AP is preparing to shut down. In this state, Garage Mode is active and idle
     * tasks are allowed to run.
     * After reporting this state, AP will accept VehicleApPowerStateReq#SHUTDOWN_PREPARE
     * requesting immediate shutdown or VehicleApPowerStateReq#CANCEL_SHUTDOWN. Other
     * power state requests are ignored.
     */
    SHUTDOWN_PREPARE = 0x7,
    /**
     * AP has stopped preparing to shut down.
     * After reporting this state, AP will accept VehicleApPowerStateReq#ON or
     * VehicleApPowerStateReq#SHUTDOWN_PREPARE. Other power state requests are ignored.
     */
    SHUTDOWN_CANCELLED = 0x8,
    /**
     * AP is ready to hibernate.
     * The AP will not send any more state reports after this.
     * After reporting this state, AP will accept VehicleApPowerStateReq#FINISHED.
     * Other power state requests are ignored.
     *
     * int32Values[1]: Time to turn AP back on, in seconds. Power controller should turn on
     *                 AP after the specified time has elapsed, so AP can run tasks like
     *                 update. If this value is 0, no wake up is requested. The power
     *                 controller may not necessarily support timed wake-up.
     */
    HIBERNATION_ENTRY = 0x9,
    /**
     * AP is exiting from hibernation state.
     * After reporting this state, AP will accept VehicleApPowerStateReq#ON or
     * VehicleApPowerStateReq#SHUTDOWN_PREPARE. Other power state requests are ignored.
     */
    HIBERNATION_EXIT = 0xA,
}
