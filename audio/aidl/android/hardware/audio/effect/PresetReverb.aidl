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
 * All parameter settings must be inside the range of Capability.Range.presetReverb definition if
 * the definition for the corresponding parameter tag exist. See more detals about Range in
 * Range.aidl.
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
     * The list of presets supported by implementation, effect implementation must declare the
     * support of Presets with Capability.Range.presetReverb definition. For example, if an effect
     * implementation supports all Presets in PresetReverb.Presets, then the capability will be:
     *  const std::vector<PresetReverb::Presets> kSupportedPresets{
     *          ndk::enum_range<PresetReverb::Presets>().begin(),
     *          ndk::enum_range<PresetReverb::Presets>().end()};
     *  const std::vector<Range::PresetReverbRange> kRanges = {
     *          MAKE_RANGE(PresetReverb, supportedPresets, kSupportedPresets, kSupportedPresets)};
     */
    Presets[] supportedPresets;

    /**
     * Get current reverb preset when used in getParameter.
     * Enable a preset reverb when used in setParameter.
     * With the current Range definition, there is no good way to define enum capability, so the
     * Presets vector supportedPresets is used to defined the capability. Client must check the
     * capability in PresetReverb.supportedPresets, and make sure to get the list of supported
     * presets before setting.
     */
    Presets preset;
}
