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

package android.hardware.bluetooth.audio;

/**
 * Used for Hardware Encoding/Decoding Aptx Adaptive LE/LEX codec configuration.
 */
@VintfStability
parcelable AptxAdaptiveLeConfiguration {
    /*
     * PCM is Input for encoder, Output for decoder
     */
    byte pcmBitDepth;
    /*
     * codec-specific parameters
     */
    int samplingFrequencyHz;
    /*
     * FrameDuration based on microseconds.
     */
    int frameDurationUs;
    /*
     * length in octets of a codec frame
     */
    int octetsPerFrame;
    /*
     * Number of blocks of codec frames per single SDU (Service Data Unit)
     */
    byte blocksPerSdu;
    /*
     * Currently being used for Aptx Adaptive LEX,
     * RFU for Aptx Adaptive LE
     * Based on this value, the codec will determine the quality of stream
     * during initialization for Music/Game
     */
    int codecMode;
}
