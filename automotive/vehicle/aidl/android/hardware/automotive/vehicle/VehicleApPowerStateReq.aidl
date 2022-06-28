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
enum VehicleApPowerStateReq {
    /**
     * This requests Android to enter its normal operating state.
     * This may be sent after the AP has reported
     * VehicleApPowerStateReport#DEEP_SLEEP_EXIT,
     * VehicleApPowerStateReport#HIBERNATION_EXIT,
     * VehicleApPowerStateReport#SHUTDOWN_CANCELLED, or
     * VehicleApPowerStateReport#WAIT_FOR_VHAL.
     */
    ON = 0,
    /**
     * The power controller issues this request to shutdown the system.
     * This may be sent after the AP has reported
     * VehicleApPowerStateReport#DEEP_SLEEP_EXIT,
     * VehicleApPowerStateReport#HIBERNATION_EXIT,
     * VehicleApPowerStateReport#ON,
     * VehicleApPowerStateReport#SHUTDOWN_CANCELLED,
     * VehicleApPowerStateReport#SHUTDOWN_POSTPONE,
     * VehicleApPowerStateReport#SHUTDOWN_PREPARE, or
     * VehicleApPowerStateReport#WAIT_FOR_VHAL.
     *
     * int32Values[1] : One of VehicleApPowerStateShutdownParam.
     *                  This parameter indicates if the AP should shut
     *                  down fully or sleep. This parameter also
     *                  indicates if the shutdown should be immediate
     *                  or if it can be postponed. If the shutdown can
     *                  be postponed, AP requests postponing by sending
     *                  VehicleApPowerStateReport#SHUTDOWN_POSTPONE.
     */
    SHUTDOWN_PREPARE = 1,
    /**
     * Cancel the shutdown.
     * This may be sent after the AP has reported
     * VehicleApPowerStateReport#SHUTDOWN_POSTPONE or
     * VehicleApPowerStateReport#SHUTDOWN_PREPARE.
     * After receiving this request, the AP will report
     * VehicleApPowerStateReport#WAIT_FOR_VHAL in preparation to going ON.
     */
    CANCEL_SHUTDOWN = 2,
    /**
     * Completes the shutdown process.
     * This may be sent after the AP has reported
     * VehicleApPowerStateReport#DEEP_SLEEP_ENTRY or
     * VehicleApPowerStateReport#HIBERNATION_ENTRY or
     * VehicleApPowerStateReport#SHUTDOWN_START. The AP will not report new
     * state information after receiving this request.
     */
    FINISHED = 3,
}
