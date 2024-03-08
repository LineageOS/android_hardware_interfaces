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

import android.hardware.bluetooth.audio.ChannelMode;

/**
 * Used to exchange generic codec parameters between the stack and the provider.
 */
@VintfStability
parcelable CodecParameters {
    /**
     * PCM related parameters:
     * - Mono, Dual-Mono or Stereo
     * - Sampling frequencies, in Hz.
     * - Fixed point resolution, basically 16, 24 or 32 bits by samples.
     *   The value 32 should be used for floating point representation..
     */
    ChannelMode channelMode;
    int samplingFrequencyHz;
    int bitdepth;

    /**
     * Encoding parameters:
     *
     * - Bitrate limits on a frame basis, defined in bits per second.
     *   The encoder bitrate mode can be encoded following this rule:
     *     . minBitrate equals to maxBitrate for constant bitrate
     *     . minBitrate set to 0, for VBR with peak bitrate at maxBitratre value.
     *     . minBitrate greater than 0, for ABR, the bitrate of the stream varies
     *       between minBitrate to maxBitrate according to link quality.
     *   The 0 value for both means "undefined" or "don't care".
     *
     * - Low-latency configuration privileged
     * - Lossless effort indication. The 'False' value can be used as "don't care"
     */
    int minBitrate;
    int maxBitrate;

    boolean lowLatency;
    boolean lossless;

    /**
     * Vendor specific parameters, inserted in the Vendor Specific HCI Command
     * `Start A2DP Offload` as it is. The stack operates as a pass-through;
     * the data SHALL NOT be inspected nor written by the client.
     * The size is limited to 128 bytes by the client; a larger size is
     * interpreted as a zero-sized buffer.
     */
    byte[] vendorSpecificParameters;
}
