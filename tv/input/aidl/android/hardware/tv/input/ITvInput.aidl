/*
 * Copyright 2022 The Android Open Source Project
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

package android.hardware.tv.input;

import android.hardware.common.NativeHandle;
import android.hardware.tv.input.ITvInputCallback;
import android.hardware.tv.input.TvStreamConfig;

@VintfStability
interface ITvInput {
    /**
     * ServiceSpecificException values for ITvInput requests
     */
    const int STATUS_UNKNOWN = 1;
    const int STATUS_NO_RESOURCE = 2;
    const int STATUS_INVALID_ARGUMENTS = 3;
    const int STATUS_INVALID_STATE = 4;

    /**
     * Closes a specific stream in a device.
     *
     * @param deviceId Device ID for the stream to close.
     * @param streamId Stream ID for the stream to close.
     * @throws ServiceSpecificException with values from the ITvInput::STATUS_* constants
     */
    void closeStream(in int deviceId, in int streamId);

    /**
     * Gets stream configurations for a specific device.
     *
     * The configs object is valid only until the next
     * STREAM_CONFIGURATIONS_CHANGED event.
     *
     * @param deviceId Device ID for the configurations.
     * @return the array of available configurations.
     * @throws ServiceSpecificException with values from the ITvInput::STATUS_* constants
     */
    TvStreamConfig[] getStreamConfigurations(in int deviceId);

    /**
     * Opens a specific stream in a device.
     *
     * @param deviceId Device ID for the stream to open.
     * @param streamId Stream ID for the stream to open. Must be one of the
     *         stream IDs returned from getStreamConfigurations().
     * @return the handle for sideband stream.
     * @throws ServiceSpecificException with values from the ITvInput::STATUS_* constants
     */
    NativeHandle openStream(in int deviceId, in int streamId);

    /**
     * Sets a callback for events.
     *
     * Note that initially no device is available in the client side, so the
     * implementation must notify all the currently available devices including
     * static devices via callback once callback is set.
     *
     * @param callback Callback object to pass events.
     * @throws ServiceSpecificException with values from the ITvInput::STATUS_* constants
     */
    void setCallback(in ITvInputCallback callback);
}
