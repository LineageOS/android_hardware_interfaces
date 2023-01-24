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

import android.hardware.health.BatteryChargingPolicy;
import android.hardware.health.BatteryHealthData;
import android.hardware.health.BatteryStatus;
import android.hardware.health.DiskStats;
import android.hardware.health.HealthInfo;
import android.hardware.health.IHealthInfoCallback;
import android.hardware.health.StorageInfo;

/**
 * IHealth manages health info and posts events on registered callbacks.
 *
 * Implementations must send health info to all callbacks periodically.
 */
@VintfStability
interface IHealth {
    /** Status code for function. The operation encounters an unknown error. */
    const int STATUS_UNKNOWN = 2;

    /**
     * Status code for function.
     * A registered callback object is dead.
     */
    const int STATUS_CALLBACK_DIED = 4;

    /**
     * Register a callback for any health info events.
     *
     * Registering a new callback must not unregister the old one; the old
     * callback remains registered until one of the following happens:
     * - A client explicitly calls {@link #unregisterCallback} to unregister it.
     * - The client process that hosts the callback dies.
     *
     * @param callback the callback to register.
     * @return If error, return service specific error with code STATUS_UNKNOWN.
     */
    void registerCallback(in IHealthInfoCallback callback);

    /**
     * Explicitly unregister a callback that is previously registered through
     * {@link #registerCallback}.
     *
     * @param callback the callback to unregister.
     * @return If error:
     *         - Return exception with code EX_ILLEGAL_ARGUMENT
     *           if callback is not registered previously,
     *         - Return service specific error with code STATUS_UNKNOWN
     *           for other errors.
     */
    void unregisterCallback(in IHealthInfoCallback callback);

    /**
     * Schedule update.
     *
     * When update() is called, the service must notify all registered callbacks
     * with the most recent health info.
     *
     * @return If error, return service specific error with code:
     *         - STATUS_CALLBACK_DIED if any registered callback is dead,
     *         - STATUS_UNKNOWN for other errors.
     */
    void update();

    /**
     * Get battery capacity in microampere-hours(µAh).
     *
     * @return battery capacity if successful.
     *         If error:
     *         - Return exception with code EX_UNSUPPORTED_OPERATION
     *           if this property is not supported
     *           (e.g. the file that stores this property does not exist),
     *         - Retrurn service specific error with code
     *           STATUS_UNKNOWN for other errors.
     */
    int getChargeCounterUah();

    /**
     * Get instantaneous battery current in microamperes(µA).
     *
     * Positive values indicate net current entering the battery from a charge
     * source, negative values indicate net current discharging from the
     * battery.
     *
     * @return instantaneous battery current if successful.
     *         If error:
     *         - Return exception with code EX_UNSUPPORTED_OPERATION
     *           if this property is not supported
     *                 (e.g. the file that stores this property does not exist),
     *         - Return service specific error with code STATUS_UNKNOWN
     *           for other errors.
     */
    int getCurrentNowMicroamps();

    /**
     * Get average battery current in microamperes(µA).
     *
     * Positive values indicate net current entering the battery from a charge
     * source, negative values indicate net current discharging from the
     * battery. The time period over which the average is computed may depend on
     * the fuel gauge hardware and its configuration.
     *
     * @return average battery current if successful.
     *         If error:
     *         - Return exception with code EX_UNSUPPORTED_OPERATION
     *           if this property is not supported
     *                 (e.g. the file that stores this property does not exist),
     *         - Return service specific error with code STATUS_UNKNOWN
     *           for other errors.
     */
    int getCurrentAverageMicroamps();

    /**
     * Get remaining battery capacity percentage of total capacity
     * (with no fractional part).
     *
     * @return remaining battery capacity if successful.
     *         If error:
     *         - Return exception with code EX_UNSUPPORTED_OPERATION
     *           if this property is not supported
     *                 (e.g. the file that stores this property does not exist),
     *         - Return service specific error with code STATUS_UNKNOWN
     *           for other errors.
     */
    int getCapacity();

    /**
     * Get battery remaining energy in nanowatt-hours.
     *
     * @return remaining energy if successful.
     *         If error:
     *         - Return exception with code EX_UNSUPPORTED_OPERATION
     *           if this property is not supported,
     *         - Return service specific error with code STATUS_UNKNOWN
     *           for other errors.
     */
    long getEnergyCounterNwh();

    /**
     * Get battery charge status.
     *
     * @return charge status if successful.
     *         If error:
     *         - Return exception with code EX_UNSUPPORTED_OPERATION
     *           if this property is not supported
     *                 (e.g. the file that stores this property does not exist),
     *         - Return service specific error with code STATUS_UNKNOWN
     *           for other errors.
     */
    BatteryStatus getChargeStatus();

    /**
     * Get storage info.
     *
     * @return vector of StorageInfo structs if successful.
     *         If error:
     *         - Return exception with code EX_UNSUPPORTED_OPERATION
     *           if this property is not supported,
     *         - Return service specific error with code STATUS_UNKNOWN
     *           for other errors.
     */
    StorageInfo[] getStorageInfo();

    /**
     * Gets disk statistics (number of reads/writes processed, number of I/O
     * operations in flight etc).
     *
     * @return vector of disk statistics if successful.
     *         The mapping is index 0->sda, 1->sdb and so on.
     *         If error:
     *         - Return exception with code EX_UNSUPPORTED_OPERATION
     *           if this property is not supported,
     *         - Return service specific error with code STATUS_UNKNOWN
     *           for other errors.
     */
    DiskStats[] getDiskStats();

    /**
     * Get Health Information.
     *
     * @return Health information if successful.
     *         If error:
     *         - Return exception with code EX_UNSUPPORTED_OPERATION
     *           if this API is not supported,
     *         - Return service specific error with code STATUS_UNKNOWN
     *           for other errors.
     */
    HealthInfo getHealthInfo();

    /**
     * Set battery charging policy
     *
     * @return If error, return service specific error with code:
     *         - Return exception with code EX_UNSUPPORTED_OPERATION
     *           if this property is not supported
     *                 (e.g. the file that stores this property does not exist),
     *         - Return status with code INVALID_OPERATION
     *           if the operation failed.
     *         - Return service specific error with code STATUS_UNKNOWN
     *           for other errors.
     */
    void setChargingPolicy(BatteryChargingPolicy in_value);

    /**
     * Get current battery charging policy
     *
     * @return current battery charging policy if successful.
     *         If error:
     *         - Return exception with code EX_UNSUPPORTED_OPERATION
     *           if this property is not supported
     *                 (e.g. the file that stores this property does not exist),
     *         - Return service specific error with code STATUS_UNKNOWN
     *           for other errors.
     */
    BatteryChargingPolicy getChargingPolicy();

    /**
     * Get battery health data
     *
     * @return Battery health data if successful.
     *         If error:
     *         - Return exception with code EX_UNSUPPORTED_OPERATION
     *           if this property is not supported
     *                 (e.g. the file that stores this property does not exist),
     *         - Return service specific error with code STATUS_UNKNOWN
     *           for other errors.
     */
    BatteryHealthData getBatteryHealthData();
}
