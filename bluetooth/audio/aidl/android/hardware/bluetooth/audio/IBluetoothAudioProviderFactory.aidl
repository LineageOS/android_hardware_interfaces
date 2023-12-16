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

import android.hardware.bluetooth.audio.AudioCapabilities;
import android.hardware.bluetooth.audio.CodecInfo;
import android.hardware.bluetooth.audio.IBluetoothAudioProvider;
import android.hardware.bluetooth.audio.SessionType;

/**
 * This factory allows a HAL implementation to be split into multiple
 * independent providers.
 *
 * When the Bluetooth stack is ready to create an audio session, it must first
 * obtain the IBluetoothAudioProvider for that session type by calling
 * openProvider().
 *
 */

@VintfStability
interface IBluetoothAudioProviderFactory {
    /**
     * Gets a list of audio capabilities for a session type.
     *
     * For software encoding, the PCM capabilities are returned.
     * For hardware encoding, the supported codecs and their capabilities are
     * returned.
     *
     * @param sessionType The session type (e.g.
     *    A2DP_SOFTWARE_ENCODING_DATAPATH).
     * @return A list containing all the capabilities
     *    supported by the sesson type. The capabilities is a list of
     *    available options when configuring the codec for the session.
     *    For software encoding it is the PCM data rate.
     *    For hardware encoding it is the list of supported codecs and their
     *    capabilities.
     *    If a provider isn't supported, an empty list should be returned.
     *    Note: Only one entry should exist per codec when using hardware
     *    encoding.
     */
    AudioCapabilities[] getProviderCapabilities(in SessionType sessionType);

    /**
     * Opens an audio provider for a session type. To close the provider, it is
     * necessary to release references to the returned provider object.
     *
     * @param sessionType The session type (e.g.
     *    LE_AUDIO_SOFTWARE_ENCODING_DATAPATH).
     *
     * @return provider The provider of the specified session type
     */
    IBluetoothAudioProvider openProvider(in SessionType sessionType);

    /**
     * General information relative to a provider
     * - An optional name
     * - A list of codec information
     * - supportsMultidirectionalCapabilities if is set to false it means each
     * session i.e. _ENCODING and _DECODING is responsible to provide
     * configuration for a single direction:
     *    _ENCODING for SINK ASE
     *    _DECODING for SOURCE ASE
     *
     * If supportsMultidirectionalCapabilities is set to true, then either
     * _ENCODING or _DECODING session can provide the configurations for either
     * direction.
     */
    @VintfStability
    parcelable ProviderInfo {
        String name;
        CodecInfo[] codecInfos;
        boolean supportsMultidirectionalCapabilities;
    }

    /**
     * Get general information relative to a provider.
     *
     * This can be called at any time, or just once during the BT stack
     * initialization.
     *
     * @param sessionType Hardware Offload provider (*_HARDWARE_OFFLOAD_*)
     * @return General information relative to the provider.
     *         The `null` value can be returned when the provider is not
     *         available
     */
    @nullable ProviderInfo getProviderInfo(in SessionType sessionType);
}
