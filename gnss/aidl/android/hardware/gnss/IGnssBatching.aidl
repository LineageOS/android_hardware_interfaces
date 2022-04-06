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

package android.hardware.gnss;

import android.hardware.gnss.IGnssBatchingCallback;

/**
 * Extended interface for GNSS Batching support.
 *
 * If this interface is supported, this batching request must be able to run in parallel with, or
 * without, non-batched location requested by the IGnss start() & stop() - i.e. both requests must
 * be handled independently, and not interfere with each other.
 *
 * For example, if a 1Hz continuous output is underway on the IGnssCallback, due to an IGnss start()
 * operation, and then a IGnssBatching start() is called for a location every 10 seconds, the newly
 * added batching request must not disrupt the 1Hz continuous location output on the IGnssCallback.
 *
 * As with GNSS Location outputs, source of location must be GNSS satellite measurements, optionally
 * using interial and baro sensors to improve relative motion filtering. No additional absolute
 * positioning information, such as WiFi derived location, may be mixed with the GNSS information.
 *
 * @hide
 */
@VintfStability
interface IGnssBatching {
    /**
     * Bit mask indicating batching supports wake up and flush when FIFO is full.
     *
     * If this flag is set, the hardware implementation must wake up the application processor when
     * the FIFO is full, and call IGnssBatchingCallback to return the locations.
     *
     * If the flag is not set, the hardware implementation must drop the oldest data when the FIFO
     * is full.
     */
    const int WAKEUP_ON_FIFO_FULL = 0x01;

    /** Options specifying the batching request. */
    @VintfStability
    parcelable Options {
        /** Time interval between samples in the location batch, in nanoseconds. */
        long periodNanos;

        /**
         * The minimum distance in meters that the batching engine should
         * accumulate before trying another GPS fix when in a challenging GPS environment.
         *
         * This is an optional field. If it is set as 0, the chipset can operate in an automatic
         * mode.
         */
        float minDistanceMeters;

        /** A bit field of Flags (WAKEUP_ON_FIFO_FULL) indicating the batching behavior. */
        int flags;
    }

    /**
     * Open the interface and provides the callback routines to the implementation of this
     * interface.
     *
     * @param callback Callback interface for IGnssBatching.
     */
    void init(in IGnssBatchingCallback callback);

    /**
     * Return the batch size (in number of GnssLocation objects) available in this hardware
     * implementation.
     *
     * If the available size is variable, for example, based on other operations consuming memory,
     * this is the minimum size guaranteed to be available for batching operations.
     *
     * This may, for example, be used by the client, to decide on the batching interval and whether
     * the AP should be woken up or not.
     *
     * @return the number of location objects supported per batch
     */
    int getBatchSize();

    /**
     * Start batching locations. This API is primarily used when the AP is asleep and the device can
     * batch locations in the hardware.
     *
     * The implementation must invoke IGnssBatchingCallback, provided in init(), to return the
     * location.
     *
     * When the buffer is full and WAKEUP_ON_FIFO_FULL is used, IGnssBatchingCallback must be called
     * to return the locations.
     *
     * When the buffer is full and WAKEUP_ON_FIFO_FULL is not set, the oldest location object is
     * dropped. In this case the AP must not be woken up. The AP would then generally be responsible
     * for using flushBatchedLocation to explicitly ask for the location as needed, to avoid it
     * being dropped.
     *
     * @param options  Options specifying the batching request.
     */
    void start(in Options options);

    /**
     * Retrieve all batched locations currently stored.
     *
     * The implementation must invoke IGnssBatchingCallback, provided in init(), to return the
     * location.
     *
     * IGnssBatchingCallback must be called in response, even if there are no locations to flush
     * (in which case the Location vector must be empty).
     *
     * Subsequent calls to flush must not return any of the locations returned in this call.
     */
    void flush();

    /**
     * Stop batching.
     */
    void stop();

    /**
     * Closes the interface. If any batch operations are in progress, they must be stopped.  If any
     * locations are in the hardware batch, they must be deleted (and not sent via callback.)
     *
     * init() may be called again, after this, if the interface is to be restored
     */
    void cleanup();
}
