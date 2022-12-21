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
 * Automatic Gain Control (AGC) is an audio pre-processor which automatically normalizes the output
 * of the captured signal by boosting or lowering input from the microphone to match a preset level
 * so that the output signal level is virtually constant. AGC can be used by applications where the
 * input signal dynamic range is not important but where a constant strong capture level is desired.
 *
 * All parameters defined in union AutomaticGainControl must be gettable and settable. The
 * capabilities defined in AutomaticGainControl.Capability can only acquired with
 * IEffect.getDescriptor() and not settable.
 */
@VintfStability
union AutomaticGainControl {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        int vendorExtensionTag;
        AutomaticGainControl.Tag commonTag;
    }

    /**
     * Vendor AutomaticGainControl implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * Capability supported by AutomaticGainControl implementation.
     */
    @VintfStability
    parcelable Capability {
        /**
         * AutomaticGainControl capability extension, vendor can use this extension in case existing
         * capability definition not enough.
         */
        ParcelableHolder extension;
        /**
         * Max fixed digital gain supported by AGC implementation in millibel.
         */
        int maxFixedDigitalGainMb;
        /**
         * Max fixed saturation margin supported by AGC implementation in millibel.
         */
        int maxSaturationMarginMb;
    }

    @VintfStability
    @Backing(type="int")
    enum LevelEstimator {
        /* Use Root Mean Square level estimator*/
        RMS = 0,
        /* Use Peak level estimator*/
        PEAK = 1,
    }

    /**
     * The AGC fixed digital gain in millibel.
     * Must never be negative, and not larger than maxFixedDigitalGainMb in capability.
     */
    int fixedDigitalGainMb;
    /*
     * Adaptive digital level estimator.
     */
    LevelEstimator levelEstimator;
    /**
     * The AGC saturation margin in millibel.
     * Must never be negative, and not larger than maxSaturationMarginMb in capability.
     */
    int saturationMarginMb;
}
