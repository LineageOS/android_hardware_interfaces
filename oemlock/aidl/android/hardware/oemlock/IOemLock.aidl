/*
 * Copyright 2020 The Android Open Source Project
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

package android.hardware.oemlock;

import android.hardware.oemlock.OemLockSecureStatus;

/*
 * The OEM lock prevents the bootloader from allowing the device to be flashed.
 *
 * Both the carrier and the device itself have a say as to whether OEM unlock is
 * allowed and both must agree that is allowed in order for unlock to be
 * possible.
 */
@VintfStability
interface IOemLock {
    /**
     * Returns a vendor specific identifier of the HAL.
     *
     * The name returned must not be interpreted by the framework but must be
     * passed to vendor code which may use it to identify the security protocol
     * used by setOemUnlockAllowedByCarrier. This allows the vendor to identify
     * the protocol without having to maintain a device-to-protocol mapping.
     *
     * @return name of the implementation and STATUS_OK if get name successfully
     */
    String getName();

    /**
     * Returns whether OEM unlock is allowed by the carrier.
     *
     * @return the current state(allowed/not allowed) of the flag
     * and STATUS_OK if the flag was successfully read.
     */
    boolean isOemUnlockAllowedByCarrier();

    /**
     * Returns whether OEM unlock ia allowed by the device.
     *
     * @return the current state(allowed/not allowed) of the flag
     * and STATUS_OK if the flag was successfully read.
     */
    boolean isOemUnlockAllowedByDevice();

    /**
     * Updates whether OEM unlock is allowed by the carrier.
     *
     * The implementation may require a vendor defined signature to prove the
     * validity of this request in order to harden its security.
     *
     * @param allowed is the new value of the flag.
     * @param signature to prove validity of this request or empty if not
     *        required.
     * @return OK if the flag was successfully updated,
     *         INVALID_SIGNATURE if a signature is required but the wrong one
     *         was provided
     *         FAILED if the update was otherwise unsuccessful.
     */
    OemLockSecureStatus setOemUnlockAllowedByCarrier(in boolean allowed, in byte[] signature);

    /**
     * Updates whether OEM unlock is allowed by the device.
     *
     * @param allowed the new value of the flag.
     * @return STATUS_OK if the flag was successfully updated.
     */
    void setOemUnlockAllowedByDevice(in boolean allowed);
}
