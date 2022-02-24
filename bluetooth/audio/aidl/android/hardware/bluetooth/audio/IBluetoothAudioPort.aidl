/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.bluetooth.audio;

import android.hardware.audio.common.SinkMetadata;
import android.hardware.audio.common.SourceMetadata;
import android.hardware.bluetooth.audio.CodecType;
import android.hardware.bluetooth.audio.LatencyMode;
import android.hardware.bluetooth.audio.PresentationPosition;

/**
 * HAL interface from the Audio HAL to the Bluetooth stack
 *
 * The Audio HAL calls methods in this interface to start, suspend, and stop
 * an audio stream. These calls return immediately and the results, if any,
 * are sent over the IBluetoothAudioProvider interface.
 *
 * Moreover, the Audio HAL can also get the presentation position of the stream
 * and provide stream metadata.
 *
 */
@VintfStability
interface IBluetoothAudioPort {
    /**
     * Get the audio presentation position.
     *
     * @return the audio presentation position
     *
     */
    PresentationPosition getPresentationPosition();

    /**
     * This indicates that the caller of this method has opened the data path
     * and wants to start an audio stream. The caller must wait for a
     * IBluetoothAudioProvider.streamStarted(Status) call.
     *
     * @param isLowLatency true if the stream being started with the latency
     * control mechanism.
     */
    void startStream(boolean isLowLatency);

    /**
     * This indicates that the caller of this method wants to stop the audio
     * stream. The data path will be closed after this call. There is no
     * callback from the IBluetoothAudioProvider interface even though the
     * teardown is asynchronous.
     */
    void stopStream();

    /**
     * This indicates that the caller of this method wants to suspend the audio
     * stream. The caller must wait for the Bluetooth process to call
     * IBluetoothAudioProvider.streamSuspended(Status). The caller still keeps
     * the data path open.
     */
    void suspendStream();

    /**
     * Called when the metadata of the stream's source has been changed.
     *
     * @param sourceMetadata Description of the audio that is played by the
     *    clients.
     */
    void updateSourceMetadata(in SourceMetadata sourceMetadata);

    /**
     * Called when the metadata of the stream's sink has been changed.
     *
     * @param sinkMetadata as passed from Audio Framework
     */
    void updateSinkMetadata(in SinkMetadata sinkMetadata);

    /**
     * Called when latency mode is changed.
     *
     * @param latencyMode latency mode from audio
     */
    void setLatencyMode(in LatencyMode latencyMode);
}
