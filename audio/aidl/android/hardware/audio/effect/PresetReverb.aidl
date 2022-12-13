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
 * PresetReverb specific definitions.
 *
 * All parameters defined in union PresetReverb must be gettable and settable. The capabilities
 * defined in PresetReverb.Capability can only acquired with IEffect.getDescriptor() and not
 * settable.
 */
@VintfStability
union PresetReverb {
    /**
     * Presets enum definition.
     */
    @VintfStability
    @Backing(type="int")
    enum Presets {
        /**
         * No reverb or reflections
         */
        NONE,
        /**
         * A small room less than five meters in length
         */
        SMALLROOM,
        /**
         * A medium room with a length of ten meters or less.
         */
        MEDIUMROOM,
        /**
         * A large-sized room suitable for live performances.
         */
        LARGEROOM,
        /**
         * A medium-sized hall.
         */
        MEDIUMHALL,
        /**
         * a large-sized hall suitable for a full orchestra.
         */
        LARGEHALL,
        /**
         * Synthesis of the traditional plate reverb.
         */
        PLATE,
    }

    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        int vendorExtensionTag;
        PresetReverb.Tag commonTag;
    }

    /**
     * Vendor PresetReverb implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * Capability supported by effect implementation.
     */
    @VintfStability
    parcelable Capability {
        VendorExtension extension;

        /**
         * List of presets supported.
         */
        Presets[] supportedPresets;
    }

    /**
     * Get current reverb preset when used in getParameter.
     * Enable a preset reverb when used in setParameter.
     */
    Presets preset;
}
