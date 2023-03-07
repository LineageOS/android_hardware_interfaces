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
 * HapticGenerator specific definitions. HapticGenerator effect provide HapticGenerator control and
 * mute/unmute functionality.
 *
 * All parameter settings must be inside the range of Capability.Range.hapticGenerator definition if
 * the definition for the corresponding parameter tag exist. See more detals about Range in
 * Range.aidl.
 */
@VintfStability
union HapticGenerator {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        VendorExtension vendorExtensionTag;
        HapticGenerator.Tag commonTag;
    }

    /**
     * Vendor HapticGenerator implementation definition for additional parameters.
     */
    VendorExtension vendor;

    @VintfStability
    @Backing(type="int")
    enum VibratorScale {
        MUTE = -100,
        VERY_LOW = -2,
        LOW = -1,
        NONE = 0,
        HIGH = 1,
        VERY_HIGH = 2,
    }

    @VintfStability
    parcelable HapticScale {
        /**
         * Audio track ID.
         */
        int id;
        /**
         * Haptic intensity.
         */
        VibratorScale scale = VibratorScale.MUTE;
    }

    /**
     * Vibrator information including resonant frequency, Q factor.
     */
    @VintfStability
    parcelable VibratorInformation {
        /**
         * Resonant frequency in Hz.
         */
        float resonantFrequencyHz;
        float qFactor;
        float maxAmplitude;
    }

    HapticScale[] hapticScales;
    VibratorInformation vibratorInfo;
}
