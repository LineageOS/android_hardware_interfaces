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

import android.hardware.tv.tuner.FrontendDvbtCoderate;
import android.hardware.tv.tuner.FrontendDvbtGuardInterval;
import android.hardware.tv.tuner.FrontendIsdbtBandwidth;
import android.hardware.tv.tuner.FrontendIsdbtMode;
import android.hardware.tv.tuner.FrontendIsdbtModulation;
import android.hardware.tv.tuner.FrontendSpectralInversion;

/**
 * Signal Settings for ISDBT Frontend.
 * @hide
 */
@VintfStability
parcelable FrontendIsdbtSettings {
    /**
     * Signal frequency in Hertz
     */
    int frequency;

    /**
     * Signal end frequency in Hertz used by scan
     */
    int endFrequency;

    FrontendSpectralInversion inversion = FrontendSpectralInversion.UNDEFINED;

    FrontendIsdbtModulation modulation = FrontendIsdbtModulation.UNDEFINED;

    FrontendIsdbtBandwidth bandwidth = FrontendIsdbtBandwidth.UNDEFINED;

    FrontendIsdbtMode mode = FrontendIsdbtMode.UNDEFINED;

    FrontendDvbtCoderate coderate = FrontendDvbtCoderate.UNDEFINED;

    FrontendDvbtGuardInterval guardInterval = FrontendDvbtGuardInterval.UNDEFINED;

    int serviceAreaId;
}
