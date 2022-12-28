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
 * Volume specific definitions. Volume effect provide volume control and mute/unmute functionality.
 *
 * All parameters defined in union Volume must be gettable and settable. The capabilities defined in
 * Volume.Capability can only acquired with IEffect.getDescriptor() and not settable.
 */
@VintfStability
union Volume {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        int vendorExtensionTag;
        Volume.Tag commonTag;
    }

    /**
     * Vendor Volume implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * Capability supported by Volume implementation.
     */
    @VintfStability
    parcelable Capability {
        /**
         * Volume capability extension, vendor can use this extension in case existing capability
         * definition not enough.
         */
        VendorExtension extension;

        /**
         * Minimum Volume level supported in dB.
         */
        int minLevelDb;

        /**
         * Maximum Volume level supported in dB.
         */
        int maxLevelDb;
    }

    /**
     * Current level in dB with supported minimum and maximum level specified in capability.
     */
    int levelDb;
    /**
     * Mute volume if true, when volume set to mute, the current level still saved and take effect
     * when unmute.
     */
    boolean mute;
}
