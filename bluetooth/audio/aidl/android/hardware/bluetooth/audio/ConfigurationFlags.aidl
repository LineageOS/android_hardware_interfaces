/*
 * Copyright 2023 The Android Open Source Project
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

/**
 * Coding fetures
 */
@VintfStability
parcelable ConfigurationFlags {
    const int NONE = 0x0000;
    /*
     * Set for the lossless configurations
     */
    const int LOSSLESS = 0x0001;
    /*
     * Set for the low latency configurations
     */
    const int LOW_LATENCY = 0x0002;
    /*
     * When set, asymmetric configuration for SINK and SOURCE can be used.
     * e.g. in GAMING mode stream for 32kHz and back channel for 16 kHz
     */
    const int ALLOW_ASYMMETRIC_CONFIGURATIONS = 0x0003;
    /*
     * Set for the spatial audio configurations
     */
    const int SPATIAL_AUDIO = 0x0004;
    /*
     * When set, BluetoothAudioProvider requests to receive ASE metadata.
     * In such case onSinkAseMetadataChanged() and onSourceAseMetadataChanged
     * will be called.
     */
    const int PROVIDE_ASE_METADATA = 0x0005;
    /*
     * Set for mono microphone configurations
     */
    const int MONO_MIC_CONFIGURATION = 0x0006;

    int bitmask;
}
