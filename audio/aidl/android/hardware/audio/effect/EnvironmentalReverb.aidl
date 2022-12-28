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
 * Environmental Reverb specific definitions.
 *
 * All parameters defined in union Environmental must be gettable and settable. The capabilities
 * * defined in EnvironmentalReverb.Capability can only acquired with IEffect.getDescriptor() and
 * not * settable.
 */

@VintfStability
union EnvironmentalReverb {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        int vendorExtensionTag;
        EnvironmentalReverb.Tag commonTag;
    }

    /**
     * Vendor EnvironmentalReverb implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * Capability supported by effect implementation.
     */
    @VintfStability
    parcelable Capability {
        VendorExtension extension;

        /**
         * Max decay time supported in millisecond.
         */
        int maxDecayTimeMs;
    }

    /**
     * Minimal possible room level in millibels.
     */
    const int MIN_ROOM_LEVEL_MB = -6000;
    /**
     * Maximum possible room level in millibels.
     */
    const int MAX_ROOM_LEVEL_MB = 0;
    /**
     * Room level apply to the reverb effect in millibels.
     */
    int roomLevelMb;

    /**
     * Minimal possible room hf level in millibels.
     */
    const int MIN_ROOM_HF_LEVEL_MB = -4000;
    /**
     * Maximum possible room hf level in millibels.
     */
    const int MAX_ROOM_HF_LEVEL_MB = 0;
    /**
     * Room HF level apply to the reverb effect in millibels.
     */
    int roomHfLevelMb;

    /**
     * Minimal possible decay time in milliseconds.
     */
    const int MIN_DECAY_TIME_MS = 100;
    /**
     * Maximum possible decay time in milliseconds.
     */
    const int MAX_DECAY_TIME_MS = 20000;
    /**
     * Delay time apply to the reverb effect in milliseconds.
     */
    int decayTimeMs;

    /**
     * Minimal possible per mille decay hf ratio.
     */
    const int MIN_DECAY_HF_RATIO_PM = 100;
    /**
     * Maximum possible per mille decay hf ratio.
     */
    const int MAX_DECAY_HF_RATIO_PM = 1000;
    /**
     * HF decay ratio in permilles.
     */
    int decayHfRatioPm;

    /**
     * Minimal possible room level in millibels.
     */
    const int MIN_LEVEL_MB = -6000;
    /**
     * Maximum possible room level in millibels.
     */
    const int MAX_LEVEL_MB = 0;
    /**
     * Reverb level in millibels.
     */
    int levelMb;

    /**
     * Minimal possible delay time in milliseconds.
     */
    const int MIN_DELAY_MS = 0;
    /**
     * Maximum possible delay time in milliseconds.
     */
    const int MAX_DELAY_MS = 65;
    /**
     * Reverb delay in milliseconds.
     */
    int delayMs;

    /**
     * Minimal possible per mille diffusion.
     */
    const int MIN_DIFFUSION_PM = 0;
    /**
     * Maximum possible per mille diffusion.
     */
    const int MAX_DIFFUSION_PM = 1000;
    /**
     * Diffusion in permilles.
     */
    int diffusionPm;

    /**
     * Minimal possible per mille density.
     */
    const int MIN_DENSITY_PM = 0;
    /**
     * Maximum possible per mille density.
     */
    const int MAX_DENSITY_PM = 1000;
    /**
     * Density in permilles.
     */
    int densityPm;

    /**
     * Bypass reverb and copy input to output if set to true.
     */
    boolean bypass;
}
