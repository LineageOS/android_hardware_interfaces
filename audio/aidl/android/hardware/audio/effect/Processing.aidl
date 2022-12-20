/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.audio.effect;

import android.hardware.audio.effect.Descriptor;
import android.media.audio.common.AudioSource;
import android.media.audio.common.AudioStreamType;
import android.media.audio.common.AudioUuid;

/**
 * List of effects which must be used for certain pre-processing or post-processing.
 */
@VintfStability
parcelable Processing {
    @VintfStability
    union Type {
        AudioStreamType streamType = AudioStreamType.INVALID;
        AudioSource source;
    }

    /**
     * Specifies the type of processing by referring to the output stream type (AudioStreamType) or
     * the input stream source (AudioSource).
     */
    Type type;
    /**
     * List of effect descriptors for this processing.
     */
    Descriptor[] ids;
}
