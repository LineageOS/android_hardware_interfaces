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

import android.hardware.audio.effect.VendorExtension;

/**
 * Noise suppression (NS) is an audio pre-processor which removes background noise from the captured
 * signal. The component of the signal considered as noise can be either stationary (car/airplane
 * engine, AC system) or non-stationary (other peoples conversations, car horn) for more advanced
 * implementations.
 *
 * All parameter settings must be inside the range of Capability.Range.noiseSuppression definition
 * if the definition for the corresponding parameter tag exist. See more detals about Range in
 * Range.aidl.
 */
@VintfStability
union NoiseSuppression {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        VendorExtension vendorExtensionTag;
        NoiseSuppression.Tag commonTag;
    }

    /**
     * Vendor NoiseSuppression implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * Different level of Noise Suppression to set.
     * As an example, webrtc have NsConfig::SuppressionLevel::k6dB applied for LOW level noise
     * suppression, NsConfig::SuppressionLevel::k12dB for MEDIUM, and
     * NsConfig::SuppressionLevel::k18dB for HIGH.
     */
    @VintfStability @Backing(type="int") enum Level { LOW, MEDIUM, HIGH, VERY_HIGH }

    /**
     * The NS level.
     */
    Level level;

    /**
     * Noise suppression type.
     */
    @VintfStability @Backing(type="int") enum Type { SINGLE_CHANNEL, MULTI_CHANNEL }

    Type type;
}
