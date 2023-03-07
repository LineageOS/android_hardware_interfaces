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
 * Bass boost is an audio effect to boost or amplify low frequencies of the sound. It is comparable
 * to a simple equalizer but limited to one band amplification in the low frequency range.
 *
 * All parameter settings must be inside the range of Capability.Range.bassBoost definition if the
 * definition for the corresponding parameter tag exist. See more detals about Range in Range.aidl.
 */
@VintfStability
union BassBoost {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        VendorExtension vendorExtensionTag;
        BassBoost.Tag commonTag;
    }

    /**
     * Vendor BassBoost implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * The per mille strength of the bass boost effect.
     *
     * If the implementation does not support per mille accuracy for setting the strength, it is
     * allowed to round the given strength to the nearest supported value. In this case {@link
     * #IEffect.getParameter()} method should return the rounded value that was actually set.
     */
    int strengthPm;
}
