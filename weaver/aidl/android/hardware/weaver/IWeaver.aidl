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

package android.hardware.weaver;

import android.hardware.weaver.WeaverConfig;
import android.hardware.weaver.WeaverReadResponse;

/**
 * Weaver provides secure storage of secret values that may only be read if the
 * corresponding key has been presented.
 *
 * The storage must be secure as the device's user authentication and encryption
 * relies on the security of these values. The cardinality of the domains of the
 * key and value must be suitably large such that they cannot be easily guessed.
 *
 * Weaver is structured as an array of slots, each containing a key-value pair.
 * Slots are uniquely identified by an ID in the range [0, `getConfig().slots`).
 */
@VintfStability
interface IWeaver {
    /**
     * Retrieves the config information for this implementation of Weaver.
     *
     * The config is static i.e. every invocation returns the same information.
     *
     * @return config data for this implementation of Weaver if status is OK,
     *         otherwise undefined.
     */
    WeaverConfig getConfig();

    /**
     * Read binder calls may return a ServiceSpecificException with the following error codes.
     */
    const int STATUS_FAILED = 1;
    const int STATUS_INCORRECT_KEY = 2;
    const int STATUS_THROTTLE = 3;

    /**
     * Attempts to retrieve the value stored in the identified slot.
     *
     * The value is only returned if the provided key matches the key stored in
     * the slot. The value is never returned if the wrong key is provided.
     *
     * Throttling must be used to limit the frequency of failed read attempts.
     * The value is only returned when throttling is not active, even if the
     * correct key is provided. If called when throttling is active, the time
     * until the next attempt can be made is returned.
     *
     * Service status return:
     *
     * OK if the value was successfully read from slot.
     * INCORRECT_KEY if the key does not match the key in the slot.
     * THROTTLE if throttling is active.
     * STATUS_FAILED if the read was unsuccessful for another reason.
     *
     * @param slotId of the slot to read from, this must be positive to be valid.
     * @param key that is stored in the slot.
     * @return The WeaverReadResponse for this read request. If the status is OK,
     * value is set to the value in the slot and timeout is 0. Otherwise, value is
     * empty and timeout is set accordingly.
     */
    WeaverReadResponse read(in int slotId, in byte[] key);

    /**
     * Overwrites the identified slot with the provided key and value.
     *
     * The new values are written regardless of the current state of the slot in
     * order to remain idempotent.
     *
     * Service status return:
     *
     * OK if the write was successfully completed.
     * FAILED if the write was unsuccessful.
     *
     * @param slotId of the slot to write to.
     * @param key to write to the slot.
     * @param value to write to slot.
     */
    void write(in int slotId, in byte[] key, in byte[] value);
}
