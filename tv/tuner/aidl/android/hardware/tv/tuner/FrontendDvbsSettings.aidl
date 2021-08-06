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

import android.hardware.tv.tuner.FrontendDvbsCodeRate;
import android.hardware.tv.tuner.FrontendDvbsModulation;
import android.hardware.tv.tuner.FrontendDvbsPilot;
import android.hardware.tv.tuner.FrontendDvbsRolloff;
import android.hardware.tv.tuner.FrontendDvbsStandard;
import android.hardware.tv.tuner.FrontendDvbsVcmMode;
import android.hardware.tv.tuner.FrontendDvbsScanType;
import android.hardware.tv.tuner.FrontendSpectralInversion;

/**
 * Signal Settings for an DVBS Frontend.
 * @hide
 */
@VintfStability
parcelable FrontendDvbsSettings {
    /**
     * Signal frequency in Hertz
     */
    long frequency;

    /**
     * Signal end frequency in Hertz used by scan
     */
    long endFrequency;

    FrontendSpectralInversion inversion = FrontendSpectralInversion.UNDEFINED;

    FrontendDvbsModulation modulation = FrontendDvbsModulation.UNDEFINED;

    FrontendDvbsCodeRate coderate;

    /**
     * Symbols per second
     */
    int symbolRate;

    FrontendDvbsRolloff rolloff = FrontendDvbsRolloff.UNDEFINED;

    FrontendDvbsPilot pilot = FrontendDvbsPilot.UNDEFINED;

    int inputStreamId;

    FrontendDvbsStandard standard = FrontendDvbsStandard.UNDEFINED;

    FrontendDvbsVcmMode vcmMode = FrontendDvbsVcmMode.UNDEFINED;

    FrontendDvbsScanType scanType = FrontendDvbsScanType.UNDEFINED;

    boolean isDiseqcRxMessage;
}
