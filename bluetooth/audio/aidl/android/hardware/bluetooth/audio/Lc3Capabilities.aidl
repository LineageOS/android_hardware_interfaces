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

import android.hardware.bluetooth.audio.ChannelMode;

/**
 * Used for Hardware Encoding/Decoding LC3 codec capabilities.
 */
@VintfStability
parcelable Lc3Capabilities {
    /*
     * PCM is Input for encoder, Output for decoder
     */
    byte[] pcmBitDepth;
    /*
     * codec-specific parameters
     */
    int[] samplingFrequencyHz;
    /*
     * FrameDuration based on microseconds.
     */
    int[] frameDurationUs;
    /*
     * length in octets of a codec frame
     */
    int[] octetsPerFrame;
    /*
     * Number of blocks of codec frames per single SDU (Service Data Unit)
     */
    byte[] blocksPerSdu;
    /*
     * Channel mode used in A2DP special audio, ignored in standard LE Audio mode
     */
    ChannelMode[] channelMode;
}
