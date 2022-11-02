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
 * Equalizer specific definitions.
 *
 * All parameters defined in union Equalizer must be gettable and settable. The capabilities defined
 * in Equalizer.Capability can only acquired with IEffect.getDescriptor() and not settable.
 */
@VintfStability
union Equalizer {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        int vendorExtensionTag;
        Equalizer.Tag commonTag;
    }

    /**
     * Vendor Equalizer implementation definition for additional parameters.
     */
    VendorExtension vendorExtension;

    /**
     * Capability MUST be supported by Equalizer implementation.
     */
    @VintfStability
    parcelable Capability {
        /**
         * Equalizer capability extension, vendor can use this extension in case existing capability
         * definition not enough.
         */
        ParcelableHolder extension;

        /**
         * Bands frequency ranges supported.
         */
        BandFrequency[] bandFrequencies;

        /**
         * Presets name and index.
         */
        Preset[] presets;
    }

    /**
     * Level setting for each band in millibels.
     */
    @VintfStability
    parcelable BandLevel {
        int index;
        int levelMb;
    }

    /**
     * Supported minimal and maximal frequency for each band in millihertz.
     */
    @VintfStability
    parcelable BandFrequency {
        int index;
        int minMh;
        int maxMh;
    }

    /**
     * Factory presets supported.
     */
    @VintfStability
    parcelable Preset {
        int index;
        /**
         * Preset name, used to identify presets but no intended to display on UI directly.
         */
        @utf8InCpp String name;
    }

    /**
     * Level for each band.
     */
    BandLevel[] bandLevels;
    /**
     * Index of current preset.
     */
    int preset;
}
