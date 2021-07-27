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

/**
 * Capabilities for DVBT Frontend.
 * @hide
 */
@VintfStability
parcelable FrontendDvbtCapabilities {
    /**
     * Transmission Modes defined by FrontendDvbtTransmissionMode.
     */
    int transmissionModeCap;

    /**
     * Bandwidth Types defined by FrontendDvbtBandwidth.
     */
    int bandwidthCap;

    /**
     * Extended Constellations defined by FrontendDvbtConstellation.
     */
    int constellationCap;

    /**
     * Code Rates defined by FrontendDvbtCoderate.
     */
    int coderateCap;

    /**
     * Hierarchy Types defined by FrontendDvbtHierarchy.
     */
    int hierarchyCap;

    /**
     * Guard Interval Types defined by FrontendDvbtGuardInterval.
     */
    int guardIntervalCap;

    boolean isT2Supported;

    boolean isMisoSupported;
}
