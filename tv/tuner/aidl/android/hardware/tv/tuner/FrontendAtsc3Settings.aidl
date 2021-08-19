/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

import android.hardware.tv.tuner.FrontendAtsc3Bandwidth;
import android.hardware.tv.tuner.FrontendAtsc3DemodOutputFormat;
import android.hardware.tv.tuner.FrontendAtsc3PlpSettings;
import android.hardware.tv.tuner.FrontendSpectralInversion;

/**
 * Signal Settings for an ATSC3 Frontend.
 * @hide
 */
@VintfStability
parcelable FrontendAtsc3Settings {
    /**
     * Signal frequency in Hertz
     */
    long frequency;

    /**
     * Signal end frequency in Hertz used by scan
     */
    long endFrequency;

    /**
     * Bandwidth of tuning band.
     */
    FrontendAtsc3Bandwidth bandwidth = FrontendAtsc3Bandwidth.UNDEFINED;

    FrontendSpectralInversion inversion = FrontendSpectralInversion.UNDEFINED;

    FrontendAtsc3DemodOutputFormat demodOutputFormat = FrontendAtsc3DemodOutputFormat.UNDEFINED;

    FrontendAtsc3PlpSettings[] plpSettings;
}
