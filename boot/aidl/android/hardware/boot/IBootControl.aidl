//
// Copyright (C) 2022 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

package android.hardware.boot;

import android.hardware.boot.MergeStatus;

@VintfStability
interface IBootControl {
    const int INVALID_SLOT = -1;
    const int COMMAND_FAILED = -2;
    /**
     * Returns the active slot to boot into on the next boot. If
     * setActiveBootSlot() has been called, the getter function should return the
     * same slot as the one provided in the last setActiveBootSlot() call.
     * The returned value is always guaranteed to be strictly less than the
     * value returned by getNumberSlots. Slots start at 0 and finish at
     * getNumberSlots() - 1. For instance, a system with A/B must return 0 or 1.
     * @return the active slot to boot into on the next boot.
     */
    int getActiveBootSlot();

    /**
     * getCurrentSlot() returns the slot number of that the current boot is booted
     * from, for example slot number 0 (Slot A). It is assumed that if the current
     * slot is A, then the block devices underlying B can be accessed directly
     * without any risk of corruption.
     * The returned value is always guaranteed to be strictly less than the
     * value returned by getNumberSlots. Slots start at 0 and finish at
     * getNumberSlots() - 1. The value returned here must match the suffix passed
     * from the bootloader, regardless of which slot is active or successful.
     * @return the slot number of that the current boot is booted
     */
    int getCurrentSlot();

    /**
     * getNumberSlots() returns the number of available slots.
     * For instance, a system with a single set of partitions must return
     * 1, a system with A/B must return 2, A/B/C -> 3 and so on. A system with
     * less than two slots doesn't support background updates, for example if
     * running from a virtual machine with only one copy of each partition for the
     * purpose of testing.
     * @return number of available slots
     */
    int getNumberSlots();

    /**
     * Returns whether a snapshot-merge of any dynamic partition is in progress.
     *
     * This function must return the merge status set by the last setSnapshotMergeStatus call and
     * recorded by the bootloader with one exception. If the partitions are being flashed from the
     * bootloader such that the pending merge must be canceled (for example, if the super partition
     * is being flashed), this function must return CANCELLED.
     *
     * @param out success True if the merge status is read successfully, false otherwise.
     * @return Merge status.
     */
    MergeStatus getSnapshotMergeStatus();

    /**
     * getSuffix() returns the string suffix used by partitions that correspond to
     * the slot number passed in as a parameter. The bootloader must pass the
     * suffix of the currently active slot either through a kernel command line
     * property at androidboot.slot_suffix, or the device tree at
     * /firmware/android/slot_suffix.
     * @return suffix for the input slot, or the empty string "" if slot
     * does not match an existing slot.
     */
    String getSuffix(in int slot);

    /**
     * isSlotBootable() returns if the slot passed in parameter is bootable. Note
     * that slots can be made unbootable by both the bootloader and by the OS
     * using setSlotAsUnbootable.
     * @return true if the slot is bootable, false if it's not.
     * @throws service specific error INVALID_SLOT if slot is invalid.
     */
    boolean isSlotBootable(in int slot);

    /**
     * isSlotMarkedSuccessful() returns if the slot passed in parameter has been
     * marked as successful using markBootSuccessful. Note that only the current
     * slot can be marked as successful but any slot can be queried.
     * @return true if the slot has been marked as successful, false if it has
     * not.
     * @throws service specific error INVALID_SLOT if slot is invalid.
     */
    boolean isSlotMarkedSuccessful(in int slot);

    /**
     * markBootSuccessful() marks the current slot as having booted successfully.
     *
     * @throws Service specific error COMMAND_FAILED if command failed.
     */
    void markBootSuccessful();

    /**
     * setActiveBootSlot() marks the slot passed in parameter as the active boot
     * slot (see getCurrentSlot for an explanation of the "slot" parameter). This
     * overrides any previous call to setSlotAsUnbootable.
     * @throws Service specific error INVALID_SLOT if slot is invalid, or COMMAND_FAILED if
     * operation failed.
     */
    void setActiveBootSlot(in int slot);

    /**
     * setSlotAsUnbootable() marks the slot passed in parameter as
     * an unbootable. This can be used while updating the contents of the slot's
     * partitions, so that the system must not attempt to boot a known bad set up.
     * @throws Service specific error INVALID_SLOT if slot is invalid, or COMMAND_FAILED if
     * operation failed.
     */
    void setSlotAsUnbootable(in int slot);

    /**
     * Sets whether a snapshot-merge of any dynamic partition is in progress.
     *
     * After the merge status is set to a given value, subsequent calls to
     * getSnapshotMergeStatus must return the set value.
     *
     * The merge status must be persistent across reboots. That is, getSnapshotMergeStatus
     * must return the same value after a reboot if the merge status is not altered in any way
     * (e.g. set by setSnapshotMergeStatus or set to CANCELLED by bootloader).
     *
     * Read/write access to the merge status must be atomic. When the HAL is processing a
     * setSnapshotMergeStatus call, all subsequent calls to getSnapshotMergeStatus must block until
     * setSnapshotMergeStatus has returned.
     *
     * A MERGING state indicates that dynamic partitions are partially comprised by blocks in the
     * userdata partition.
     *
     * When the merge status is set to MERGING, the following operations must be prohibited from the
     * bootloader:
     *  - Flashing or erasing "userdata" or "metadata".
     *
     * The following operations may be prohibited when the status is set to MERGING. If not
     * prohibited, it is recommended that the user receive a warning.
     *  - Changing the active slot (e.g. via "fastboot set_active")
     *
     * @param status Merge status.
     *
     * @throws service specific error COMMAND_FAILED if operation failed.
     */
    void setSnapshotMergeStatus(in MergeStatus status);
}
