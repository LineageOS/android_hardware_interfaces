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

package android.hardware.dumpstate;

import android.os.ParcelFileDescriptor;

@VintfStability
interface IDumpstateDevice {
    /**
     * Constants that define the type of bug report being taken to restrict content appropriately.
     */
    @VintfStability
    @Backing(type="int")
    enum DumpstateMode {
        /**
         * Takes a bug report without user interference.
         */
        FULL = 0,
        /**
         * Interactive bug report, i.e. triggered by the user.
         */
        INTERACTIVE = 1,
        /**
         * Remote bug report triggered by DevicePolicyManager, for example.
         */
        REMOTE = 2,
        /**
         * Bug report triggered on a wear device.
         */
        WEAR = 3,
        /**
         * Bug report limited to only connectivity info (cellular, wifi, and networking). Sometimes
         * called "telephony" in legacy contexts.
         *
         * All reported information MUST directly relate to connectivity debugging or customer
         * support and MUST NOT contain unrelated private information. This information MUST NOT
         * identify user-installed packages (UIDs are OK, package names are not), and MUST NOT
         * contain logs of user application traffic.
         */
        CONNECTIVITY = 4,
        /**
         * Bug report limited to only wifi info.
         */
        WIFI = 5,
        /**
         * Default mode, This mode MUST be supported if the
         * dumpstate HAL is implemented.
         */
        DEFAULT = 6,
        /**
         * Takes a report in protobuf.
         *
         * The content, if implemented, must be a binary protobuf message written to the first file
         * descriptor of the native handle. The protobuf schema shall be defined by the vendor.
         */
        PROTO = 7,
    }

    /**
     * Returned for cases where the device doesn't support the given DumpstateMode (e.g. a phone
     * trying to use DumpstateMode::WEAR).
     */
    const int ERROR_UNSUPPORTED_MODE = 1;
    /**
     * Returned when device logging is not enabled.
     */
    const int ERROR_DEVICE_LOGGING_NOT_ENABLED = 2;

    /**
     * Dump device-specific state into the given file descriptors.
     *
     * One file descriptor must be passed to this method but two may be passed:
     * the first descriptor must be used to dump device-specific state in text
     * format, the second descriptor is optional and may be used to dump
     * device-specific state in binary format.
     *
     * DumpstateMode can be used to limit the information that is output.
     * For an example of when this is relevant, consider a bug report being generated with
     * DumpstateMode::CONNECTIVITY - there is no reason to include camera or USB logs in this type
     * of report.
     *
     * When verbose logging is disabled, getVerboseLoggingEnabled returns false, and this
     * API is called, it may still output essential information but must not include
     * information that identifies the user.
     *
     * @param fd array of file descriptors, with one or two valid file descriptors. The first FD is
     *         for text output, the second (if present) is for binary output.
     * @param mode A mode value to restrict dumped content.
     * @param timeoutMillis An approximate "budget" for how much time this call has been allotted.
     *     If execution runs longer than this, the IDumpstateDevice service may be killed and only
     *     partial information will be included in the report.
     * @throws ServiceSpecificException with one of the following values:
     *         |ERROR_UNSUPPORTED_MODE|,
     *         |ERROR_DEVICE_LOGGING_NOT_ENABLED|
     */
    void dumpstateBoard(in ParcelFileDescriptor[] fd, in DumpstateMode mode, in long timeoutMillis);

    /**
     * Queries the current state of verbose device logging. Primarily for UI and informative
     * purposes.
     *
     * Even if verbose logging has been disabled, dumpstateBoard may still be called by the
     * dumpstate routine, and essential information that does not identify the user may be included.
     *
     * @return Whether or not verbose vendor logging is currently enabled.
     */
    boolean getVerboseLoggingEnabled();

    /**
     * Turns verbose device vendor logging on or off.
     *
     * The setting should be persistent across reboots. Underlying implementations may need to start
     * vendor logging daemons, set system properties, or change logging masks, for example. Given
     * that many vendor logs contain significant amounts of private information and may come with
     * memory/storage/battery impacts, calling this method on a user build should only be done after
     * user consent has been obtained, e.g. from a toggle in developer settings.
     *
     * Even if verbose logging has been disabled, dumpstateBoard may still be called by the
     * dumpstate routine, and essential information that does not identify the user may be included.
     *
     * @param enable Whether to enable or disable verbose vendor logging.
     */
    void setVerboseLoggingEnabled(in boolean enable);
}
