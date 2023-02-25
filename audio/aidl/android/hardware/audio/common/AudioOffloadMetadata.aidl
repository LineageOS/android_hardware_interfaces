/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.audio.common;

import android.media.audio.common.AudioChannelLayout;

/**
 * Dynamic metadata for offloaded compressed audio.
 * For static metadata, see android.media.audio.common.AudioOffloadInfo.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable AudioOffloadMetadata {
    int sampleRate;
    AudioChannelLayout channelMask;
    /** Average bit rate in bits per second. */
    int averageBitRatePerSecond;
    /**
     * Number of frames to be ignored at the beginning of the stream.
     * The value must be non-negative. A value of 0 indicates no delay
     * has to be applied.
     */
    int delayFrames;
    /**
     * Number of frames to be ignored at the end of the stream.
     * The value must be non-negative. A value of 0 indicates no padding
     * has to be applied.
     */
    int paddingFrames;
}
