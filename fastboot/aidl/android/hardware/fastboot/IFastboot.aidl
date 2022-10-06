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

package android.hardware.fastboot;

import android.hardware.fastboot.FileSystemType;

/**
 * IFastboot interface implements vendor specific fastboot commands.
 */
@VintfStability
interface IFastboot {
    /**
     * Status code for function.
     * Operation failed due to unknown reason.
     */
    const int FAILURE_UNKNOWN = 1;
    /**
     * Executes a fastboot OEM command.
     *
     * @param oemCmd The oem command that is passed to the fastboot HAL.
     * @return optional String if the operation is successful and output is expected
     *         for the command.
     * @throws :
     *         - EX_ILLEGAL_ARGUMENT for bad arguments.
     *         - EX_UNSUPPORTED_OPERATION for unsupported commands.
     *         - EX_SERVICE_SPECIFIC with status FAILURE_UNKNOWN for other errors.
     */
    String doOemCommand(in String oemCmd);

    /**
     * Executes an OEM specific erase after fastboot erase userdata.
     *
     * @throws :
     *         - EX_UNSUPPORTED_OPERATION if it is not supported.
     *         - EX_SERVICE_SPECIFIC with status FAILURE_UNKNOWN for
     *           unknown error in oem specific command or other errors.
     */
    void doOemSpecificErase();

    /**
     * Returns the minimum battery voltage required for flashing in mV.
     *
     * @return Minimum batterery voltage (in mV) required for flashing to
     *         be successful.
     * @throws :
     *         - EX_SERVICE_SPECIFIC with status FAILURE_UNKNOWN if error.
     */
    int getBatteryVoltageFlashingThreshold();

    /**
     * Returns whether off-mode-charging is enabled. If enabled, the device
     *      autoboots into a special mode when power is applied.
     *
     * @return Returns whether off-mode-charging is enabled.
     * @throws :
     *         - EX_SERVICE_SPECIFIC with status FAILURE_UNKNOWN if error.
     */
    boolean getOffModeChargeState();

    /**
     * Returns the file system type of the partition. Implementation is only
     *       required for physical partitions that need to be wiped and reformatted.
     * @param in partitionName Name of the partition.
     * @return Returns the file system type of the partition. Type can be ext4,
     *         f2fs or raw.
     * @throws :
     *         - EX_SERVICE_SPECIFIC with status FAILURE_UNKNOWN if the partition
     *           is invalid or does not require reformatting.
     */
    FileSystemType getPartitionType(in String partitionName);

    /**
     * Returns an OEM-defined string indicating the variant of the device, for
     * example, US and ROW.
     * @return Indicates the device variant.
     * @throws :
     *         - EX_SERVICE_SPECIFIC with status FAILURE_UNKNOWN if error.
     */
    String getVariant();
}
