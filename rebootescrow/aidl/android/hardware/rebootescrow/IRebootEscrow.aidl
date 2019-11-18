/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.hardware.rebootescrow;

/**
 * This HAL defines the interface to the device-specific implementation
 * of retaining a secret to unlock the Synthetic Password stored during
 * a reboot to perform an OTA update. The implementation of this interface
 * should never store the key on any non-volatile medium. The key should be
 * overwritten with zeroes when destroyKey() is called. All care should be given
 * to provide the shortest lifetime for the storage of the key in volatile and
 * erasable storage.
 *
 * This HAL is optional so does not require an implementation on device.
 */
@VintfStability
interface IRebootEscrow {
    /**
     * Store the key for reboot.
     */
    void storeKey(in byte[] kek);

    /**
     * Retrieve the possible keys. If the implementation is probabalistic, it
     * should return the keys in order from most-probable to least-probable.
     * There is not a hard limit to the number of keys, but it is suggested to
     * keep the number of key possibilities less than 32.
     */
    byte[] retrieveKey();
}
