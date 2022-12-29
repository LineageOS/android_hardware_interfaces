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
 * All parameters defined in union BassBoost must be gettable and settable. The capabilities defined
 * in BassBoost.Capability can only acquired with IEffect.getDescriptor() and not settable.
 */
@VintfStability
union BassBoost {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        int vendorExtensionTag;
        BassBoost.Tag commonTag;
    }

    /**
     * Vendor BassBoost implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * Capability supported by BassBoost implementation.
     */
    @VintfStability
    parcelable Capability {
        /**
         * BassBoost capability extension, vendor can use this extension in case existing capability
         * definition not enough.
         */
        ParcelableHolder extension;
        /**
         * Maximum possible per mille strength.
         */
        int maxStrengthPm;
        /**
         * Indicates whether setting strength is supported. False value indicates only one strength
         * is supported and setParameter() method will return EX_ILLEGAL_ARGUMENT.
         */
        boolean strengthSupported;
    }

    /**
     * The per mille strength of the bass boost effect.
     *
     * If the implementation does not support per mille accuracy for setting the strength, it is
     * allowed to round the given strength to the nearest supported value. In this case {@link
     * #IEffect.getParameter()} method should return the rounded value that was actually set.
     *
     * The value of the strength must be non-negative and not exceed the value specified by
     * the 'maxStrengthPm' capability.
     */
    int strengthPm;
}
