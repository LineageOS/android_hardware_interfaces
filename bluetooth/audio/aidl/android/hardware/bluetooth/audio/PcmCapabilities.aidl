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
 * Used for Software Encoding audio feed capabilities
 */
@VintfStability
parcelable PcmCapabilities {
    int[] sampleRateHz;
    ChannelMode[] channelMode;
    byte[] bitsPerSample;
    /**
     * Data interval for data transfer
     */
    int[] dataIntervalUs;
}
