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

import android.hardware.tv.tuner.FrontendDvbtBandwidth;
import android.hardware.tv.tuner.FrontendDvbtCoderate;
import android.hardware.tv.tuner.FrontendDvbtConstellation;
import android.hardware.tv.tuner.FrontendDvbtGuardInterval;
import android.hardware.tv.tuner.FrontendDvbtHierarchy;
import android.hardware.tv.tuner.FrontendDvbtPlpMode;
import android.hardware.tv.tuner.FrontendDvbtStandard;
import android.hardware.tv.tuner.FrontendDvbtTransmissionMode;
import android.hardware.tv.tuner.FrontendSpectralInversion;

/**
 * Signal Settings for DVBT Frontend.
 * @hide
 */
@VintfStability
parcelable FrontendDvbtSettings {
    /**
     * Signal frequency in Hertz
     */
    long frequency;

    /**
     * Signal end frequency in Hertz used by scan
     */
    long endFrequency;

    FrontendSpectralInversion inversion = FrontendSpectralInversion.UNDEFINED;

    FrontendDvbtTransmissionMode transmissionMode = FrontendDvbtTransmissionMode.UNDEFINED;

    FrontendDvbtBandwidth bandwidth = FrontendDvbtBandwidth.UNDEFINED;

    FrontendDvbtConstellation constellation = FrontendDvbtConstellation.UNDEFINED;

    FrontendDvbtHierarchy hierarchy = FrontendDvbtHierarchy.UNDEFINED;

    /**
     * Code Rate for High Priority level
     */
    FrontendDvbtCoderate hpCoderate = FrontendDvbtCoderate.UNDEFINED;

    /**
     * Code Rate for Low Priority level
     */
    FrontendDvbtCoderate lpCoderate = FrontendDvbtCoderate.UNDEFINED;

    FrontendDvbtGuardInterval guardInterval = FrontendDvbtGuardInterval.UNDEFINED;

    boolean isHighPriority;

    FrontendDvbtStandard standard = FrontendDvbtStandard.UNDEFINED;

    boolean isMiso;

    FrontendDvbtPlpMode plpMode = FrontendDvbtPlpMode.UNDEFINED;

    /**
     * Physical Layer Pipe (PLP) Id
     */
    int plpId;

    /**
     * Group Id for Physical Layer Pipe (PLP)
     */
    int plpGroupId;
}
