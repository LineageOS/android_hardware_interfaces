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

import android.hardware.bluetooth.audio.A2dpConfiguration;
import android.hardware.bluetooth.audio.A2dpConfigurationHint;
import android.hardware.bluetooth.audio.A2dpRemoteCapabilities;
import android.hardware.bluetooth.audio.A2dpStatus;
import android.hardware.bluetooth.audio.AudioConfiguration;
import android.hardware.bluetooth.audio.BluetoothAudioStatus;
import android.hardware.bluetooth.audio.CodecId;
import android.hardware.bluetooth.audio.CodecParameters;
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
    MQDescriptor<byte, SynchronizedReadWrite> startSession(in IBluetoothAudioPort hostIf,
            in AudioConfiguration audioConfig, in LatencyMode[] supportedLatencyModes);
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

    /**
     * Validate and parse an A2DP Configuration,
     * shall be used with A2DP session types
     *
     * @param codecId Identify the codec
     * @param The configuration as defined by the A2DP's `Codec Specific
     *        Information Elements`, or `Vendor Specific Value` when CodecId
     *        format is set to `VENDOR`.
     * @param codecParameters result of parsing, when the validation succeeded.
     * @return A2DP Status of the parsing
     */
    A2dpStatus parseA2dpConfiguration(
            in CodecId codecId, in byte[] configuration, out CodecParameters codecParameters);

    /**
     * Return a configuration, from a list of remote Capabilites,
     * shall be used with A2DP session types
     *
     * @param remoteCapabilities The capabilities of the remote device
     * @param hint Hint on selection (audio context and/or codec)
     * @return The requested configuration. A null value value is returned
     *         when no suitable configuration has been found.
     */
    @nullable A2dpConfiguration getA2dpConfiguration(
            in List<A2dpRemoteCapabilities> remoteA2dpCapabilities, in A2dpConfigurationHint hint);
}
