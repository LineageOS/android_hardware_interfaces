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
         * Minimal possible room level in millibels.
         */
        int minRoomLevelMb;
        /**
         * Maximum possible room level in millibels.
         */
        int maxRoomLevelMb;
        /**
         * Minimal possible room hf level in millibels.
         */
        int minRoomHfLevelMb;
        /**
         * Maximum possible room hf level in millibels.
         */
        int maxRoomHfLevelMb;
        /**
         * Max decay time supported in millisecond.
         */
        int maxDecayTimeMs;
        /**
         * Minimal possible per mille decay hf ratio.
         */
        int minDecayHfRatioPm;
        /**
         * Maximum possible per mille decay hf ratio.
         */
        int maxDecayHfRatioPm;
        /**
         * Minimal possible room level in millibels.
         */
        int minLevelMb;
        /**
         * Maximum possible room level in millibels.
         */
        int maxLevelMb;
        /**
         * Maximum possible delay time in milliseconds.
         */
        int maxDelayMs;
        /**
         * Maximum possible per mille diffusion.
         */
        int maxDiffusionPm;
        /**
         * Maximum possible per mille density.
         */
        int maxDensityPm;
    }

    /**
     * Room level apply to the reverb effect in millibels. The value of the roomLevelMb must be in
     * range of the value specified by the 'minRoomLevelMb' capability and the 'maxRoomLevelMb'
     * capability.
     */
    int roomLevelMb;
    /**
     * Room HF level apply to the reverb effect in millibels. The value of the roomHfLevelMb must be
     * in range of the value specified by the 'minRoomHfLevelMb' capability and the
     * 'maxRoomHfLevelMb' capability.
     */
    int roomHfLevelMb;
    /**
     * Delay time apply to the reverb effect in milliseconds.The value of the decayTimeMs must
     * be non-negative and not exceed the value specified by the 'maxDecayTimeMs' capability.
     */
    int decayTimeMs;
    /**
     * HF decay ratio in permilles. The value of the decayHfRatioPm must be in range
     * of the value specified by the 'minDecayHfRatioPm' capability and the 'maxDecayHfRatioPm'
     * capability.
     */
    int decayHfRatioPm;
    /**
     * Reverb level in millibels. The value of the levelMb must be in range
     * of the value specified by the 'minLevelMb' capability and the 'maxLevelMb' capability.
     */
    int levelMb;
    /**
     * Reverb delay in milliseconds. The value of the delayMs must be non-negative and not
     * exceed the value specified by the 'maxDelayMs' capability.
     */
    int delayMs;
    /**
     * Diffusion in permilles. The value of the diffusionPm must be non-negative and not
     * exceed the value specified by the 'maxDiffusionPm' capability.
     */
    int diffusionPm;
    /**
     * Density in permilles. The value of the densityPm must be non-negative and not
     * exceed the value specified by the 'maxDensityPm' capability.
     */
    int densityPm;

    /**
     * Bypass reverb and copy input to output if set to true.
     */
    boolean bypass;
}
