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
 * LoudnessEnhancer specific definitions.
 *
 * All parameter settings must be inside the range of Capability.Range.loudnessEnhancer definition
 * if the definition for the corresponding parameter tag exist. See more detals about Range in
 * Range.aidl.
 */
@VintfStability
union LoudnessEnhancer {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        VendorExtension vendorExtensionTag;
        LoudnessEnhancer.Tag commonTag;
    }

    /**
     * Vendor LoudnessEnhancer implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * The maximum gain in millibels (mB) applied to the signal to process, default value is 0 which
     * corresponds to no amplification.
     */
    int gainMb;
}
