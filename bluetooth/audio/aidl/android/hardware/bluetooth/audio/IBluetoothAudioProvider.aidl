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

import android.hardware.bluetooth.audio.AudioConfiguration;
import android.hardware.bluetooth.audio.BluetoothAudioStatus;
import android.hardware.bluetooth.audio.IBluetoothAudioPort;
import android.hardware.bluetooth.audio.LatencyMode;
import android.hardware.common.fmq.MQDescriptor;
import android.hardware.common.fmq.SynchronizedReadWrite;

/**
 * HAL interface from the Bluetooth stack to the Audio HAL
 *
 * The Bluetooth stack calls methods in this interface to start and end audio
 * sessions and sends callback events to the Audio HAL.
 *
 */
@VintfStability
interface IBluetoothAudioProvider {
    /**
     * Ends the current session and unregisters the IBluetoothAudioPort
     * interface.
     */
    void endSession();

    /**
     * This method indicates that the Bluetooth stack is ready to stream audio.
     * It registers an instance of IBluetoothAudioPort with and provides the
     * current negotiated codec to the Audio HAL. After this method is called,
     * the Audio HAL can invoke IBluetoothAudioPort.startStream().
     *
     * Note: endSession() must be called to unregister this IBluetoothAudioPort
     *
     * @param hostIf An instance of IBluetoothAudioPort for stream control
     * @param audioConfig The audio configuration negotiated with the remote
     *    device. The PCM parameters are set if software based encoding,
     *    otherwise the correct codec configuration is used for hardware
     *    encoding.
     * @param supportedLatencyModes latency modes supported by the active
     * remote device
     *
     * @return The fast message queue for audio data from/to this
     *    provider. Audio data will be in PCM format as specified by the
     *    audioConfig.pcmConfig parameter. Invalid if streaming is offloaded
     *    from/to hardware or on failure
     */
    MQDescriptor<byte, SynchronizedReadWrite> startSession(
            in IBluetoothAudioPort hostIf, in AudioConfiguration audioConfig,
            in LatencyMode[] supportedLatencyModes);
    /**
     * Callback for IBluetoothAudioPort.startStream()
     *
     * @param status true for SUCCESS or false for FAILURE
     */
    void streamStarted(in BluetoothAudioStatus status);

    /**
     * Callback for IBluetoothAudioPort.suspendStream()
     *
     * @param status true for SUCCESS or false for FAILURE
     */
    void streamSuspended(in BluetoothAudioStatus status);

    /**
     * Called when the audio configuration of the stream has been changed.
     *
     * @param audioConfig The audio configuration negotiated with the remote
     *    device. The PCM parameters are set if software based encoding,
     *    otherwise the correct codec configuration is used for hardware
     *    encoding.
     */
    void updateAudioConfiguration(in AudioConfiguration audioConfig);

    /**
     * Called when the supported latency mode is updated.
     *
     * @param allowed If the peripheral devices can't keep up with low latency
     * mode, the API will be called with supported is false.
     */
    void setLowLatencyModeAllowed(in boolean allowed);
}
