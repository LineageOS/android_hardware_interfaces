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

import android.hardware.audio.effect.Equalizer;

/**
 * Effect capability definitions.
 * This data structure is used as part of effect Descriptor to identify effect capabilities which
 * not meant to change at runtime.
 */
@VintfStability
union Capability {
    /**
     * Vendor defined effect capability.
     * This extension can be used when vendor have a new effect implementated and need
     * capability definition for this new type of effect.
     * If vendor want to extend existing effect capabilities, it is recommended to expose though
     * the ParcelableHolder in each effect capability definition. For example:
     * Equalizer.Capability.extension.
     */
    @VintfStability
    parcelable VendorEffectCapability {
        ParcelableHolder extension;
    }
    VendorEffectCapability vendor;

    /**
     * Equalizer capability definition.
     */
    Equalizer.Capability equalizer;
}
