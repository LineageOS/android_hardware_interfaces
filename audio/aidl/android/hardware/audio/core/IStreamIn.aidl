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

package android.hardware.audio.core;

import android.hardware.audio.common.SinkMetadata;

/**
 * This interface provides means for receiving audio data from input devices.
 */
@VintfStability
interface IStreamIn {
    /**
     * Close the stream.
     *
     * Releases any resources allocated for this stream on the HAL module side.
     * This includes the fast message queues and shared memories returned via
     * the StreamDescriptor. Thus, the stream can not be operated anymore after
     * it has been closed. The client needs to release the audio data I/O
     * objects after the call to this method returns.
     *
     * Methods of this interface throw EX_ILLEGAL_STATE for a closed stream.
     *
     * @throws EX_ILLEGAL_STATE If the stream has already been closed.
     */
    void close();

    /**
     * Update stream metadata.
     *
     * Updates the metadata initially provided at the stream creation.
     *
     * @param sinkMetadata Updated metadata.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     */
    void updateMetadata(in SinkMetadata sinkMetadata);
}
