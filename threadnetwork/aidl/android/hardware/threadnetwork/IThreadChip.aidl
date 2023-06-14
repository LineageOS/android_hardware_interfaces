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

package android.hardware.threadnetwork;

import android.hardware.threadnetwork.IThreadChipCallback;

/**
 * Controls a Thread radio chip on the device.
 */

@VintfStability
interface IThreadChip {
    /**
     * The operation failed for the internal error.
     */
    const int ERROR_FAILED = 1;

    /**
     * The invalid arguments.
     */
    const int ERROR_INVALID_ARGS = 2;

    /**
     * Insufficient buffers available to send frames.
     */
    const int ERROR_NO_BUFS = 3;

    /**
     * Service is busy and could not service the operation.
     */
    const int ERROR_BUSY = 4;

    /**
     * This method initializes the Thread HAL instance. If open completes
     * successfully, then the Thread HAL instance is ready to accept spinel
     * messages through sendSpinelFrame() API.
     *
     * @param callback  A IThreadChipCallback callback instance.
     *
     * @throws ServiceSpecificException with one of the following values:
     *     - ERROR_FAILED        The interface cannot be opened due to an internal error.
     *     - ERROR_INVALID_ARGS  The callback handle is invalid (for example, it is null).
     *     - ERROR_BUSY          This interface is in use.
     */
    void open(in IThreadChipCallback callback);

    /**
     * Close the Thread HAL instance. Must free all resources.
     */
    void close();

    /**
     * This method resets the Thread HAL internal state. The callback registered by
     * `open()` won’t be reset and the resource allocated by `open()` won’t be free.
     *
     */
    void reset();

    /**
     * This method sends a spinel frame to the Thread HAL.
     *
     * This method should block until the frame is sent out successfully or
     * the method throws errors immediately.
     *
     * Spinel Protocol:
     *     https://github.com/openthread/openthread/blob/main/src/lib/spinel/spinel.h
     *
     * @param frame  The spinel frame to be sent.
     *
     * @throws ServiceSpecificException with one of the following values:
     *         - ERROR_FAILED The Thread HAL failed to send the frame for an internal reason.
     *         - ERROR_NO_BUFS Insufficient buffer space to send the frame.
     *         - ERROR_BUSY The Thread HAL is busy.
     */
    void sendSpinelFrame(in byte[] frame);
}
