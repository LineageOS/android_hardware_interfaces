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
 * Capabilities for DTMB Frontend.
 * @hide
 */
@VintfStability
parcelable FrontendDtmbCapabilities {
    /**
     * Transmission Modes defined by FrontendDtmbTransmissionMode.
     */
    int transmissionModeCap;

    /**
     * Bandwidth Types defined by FrontendDtmbBandwidth.
     */
    int bandwidthCap;

    /**
     * Modulations defined by FrontendDtmbModulation.
     */
    int modulationCap;

    /**
     * CODERATE Types defined by FrontendDtmbCodeRate.
     */
    int codeRateCap;

    /**
     * Guard Interval Types defined by FrontendDtmbGuardInterval.
     */
    int guardIntervalCap;

    /**
     * Time Interleave Mode Type defined by FrontendDtmbTimeInterleaveMode.
     */
    int interleaveModeCap;
}
