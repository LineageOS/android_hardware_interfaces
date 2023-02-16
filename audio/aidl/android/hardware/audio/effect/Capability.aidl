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

import android.hardware.audio.effect.Range;
import android.hardware.audio.effect.VendorExtension;

/**
 * Effect capability definitions.
 * This data structure is used as part of effect Descriptor to identify effect capabilities which
 * not meant to change at runtime.
 */
@VintfStability
parcelable Capability {
    /**
     * Vendor defined effect capability.
     * This extension can be used when vendor has a new effect implementation and needs
     * capability definition for this new type of effect.
     * If vendor want to extend existing effect capabilities, it is recommended to expose through
     * the ParcelableHolder in each effect definition. For example: Equalizer.vendorExtension. And
     * define an appropriate Range for the extension.
     */
    VendorExtension vendorExtension;

    /**
     * Supported range for parameters. See Range.aidl.
     */
    Range range;
}
