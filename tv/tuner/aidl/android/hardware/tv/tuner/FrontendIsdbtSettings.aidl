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

import android.hardware.tv.tuner.FrontendIsdbtBandwidth;
import android.hardware.tv.tuner.FrontendIsdbtGuardInterval;
import android.hardware.tv.tuner.FrontendIsdbtLayerSettings;
import android.hardware.tv.tuner.FrontendIsdbtMode;
import android.hardware.tv.tuner.FrontendIsdbtPartialReceptionFlag;
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
    long frequency;

    /**
     * Signal end frequency in Hertz used by scan
     */
    long endFrequency;

    FrontendSpectralInversion inversion = FrontendSpectralInversion.UNDEFINED;

    FrontendIsdbtBandwidth bandwidth = FrontendIsdbtBandwidth.UNDEFINED;

    FrontendIsdbtMode mode = FrontendIsdbtMode.UNDEFINED;

    FrontendIsdbtGuardInterval guardInterval = FrontendIsdbtGuardInterval.UNDEFINED;

    int serviceAreaId;

    FrontendIsdbtPartialReceptionFlag partialReceptionFlag = FrontendIsdbtPartialReceptionFlag.UNDEFINED;

    FrontendIsdbtLayerSettings[] layerSettings;
}
