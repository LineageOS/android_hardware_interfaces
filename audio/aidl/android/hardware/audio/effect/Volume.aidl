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
 * All parameter settings must be inside the range of Capability.Range.volume definition if the
 * definition for the corresponding parameter tag exist. See more detals about Range in Range.aidl.
 */
@VintfStability
union Volume {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        VendorExtension vendorExtensionTag;
        Volume.Tag commonTag;
    }

    /**
     * Vendor Volume implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * Current level in dB.
     */
    int levelDb;
    /**
     * Mute volume if true, when volume set to mute, the current level still saved and take effect
     * when unmute.
     */
    boolean mute;
}
