/*
 * Copyright (C) 2021 The Android Open Source Project
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
import android.media.audio.common.AudioContentType;
import android.media.audio.common.AudioDevice;
import android.media.audio.common.AudioSource;

/**
 * Metadata of a record track for an input stream.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable RecordTrackMetadata {
    AudioSource source = AudioSource.SYS_RESERVED_INVALID;
    /**
     * Non-negative linear gain (scaling) applied to track samples.
     * 0 means muted, 1 is unity gain, 2 means double amplitude, etc.
     */
    float gain;
    /**
     * Indicates the destination of an input stream, can be left unspecified.
     */
    @nullable AudioDevice destinationDevice;
    AudioChannelLayout channelMask;
    /**
     * Tags from AudioRecord audio attributes. Tag is an additional use case
     * qualifier complementing AudioUsage and AudioContentType. Tags are set by
     * vendor specific applications and must be prefixed by "VX_". Vendor must
     * namespace their tag names to avoid conflicts, for example:
     * "VX_GOOGLE_VR". At least 3 characters are required for the vendor
     * namespace.
     */
    @utf8InCpp String[] tags;
}
