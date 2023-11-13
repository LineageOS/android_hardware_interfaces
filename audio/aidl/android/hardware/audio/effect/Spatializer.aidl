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

package android.hardware.audio.effect;

import android.hardware.audio.effect.VendorExtension;
import android.media.audio.common.AudioChannelLayout;
import android.media.audio.common.HeadTracking;
import android.media.audio.common.Spatialization;

/**
 * Union representing parameters for audio spatialization effects.
 *
 * Sound spatialization simulates sounds around the listener as if they were emanating from virtual
 * positions based on the original recording.
 * For more details, refer to the documentation:
 * https://developer.android.com/reference/android/media/Spatializer.
 *
 * android.hardware.audio.effect.Spatializer specifies parameters for the implementation of audio
 * spatialization effects.
 *
 * A Spatializer implementation must report its supported parameter ranges using Capability.Range.
 * spatializer.
 */
@VintfStability
union Spatializer {
    /**
     * Parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        VendorExtension vendorExtensionTag;
        Spatializer.Tag commonTag;
    }

    /**
     * Vendor extension implementation for additional parameters.
     */
    VendorExtension vendor;

    /**
     * Level of spatialization.
     */
    Spatialization.Level spatializationLevel;

    /**
     * Head tracking mode for spatialization.
     */
    HeadTracking.Mode headTrackingMode;

    /**
     * List of supported input channel layouts.
     */
    AudioChannelLayout[] supportedChannelLayout;

    /**
     * Spatialization mode, Binaural or Transaural for example.
     */
    Spatialization.Mode spatializationMode;

    /**
     * Vector representing of the head-to-stage pose with six floats: first three are a translation
     * vector, and the last three are a rotation vector.
     */
    const int HEAD_TO_STAGE_VEC_SIZE = 6;
    float[HEAD_TO_STAGE_VEC_SIZE] headToStage;
}
