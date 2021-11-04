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
 * Capabilities for ISDBT Frontend.
 * @hide
 */
@VintfStability
parcelable FrontendIsdbtCapabilities {
    /**
     * Modes defined by FrontendIsdbtMode.
     */
    int modeCap;

    /**
     * Bandwidths defined by FrontendIsdbtBandwidth.
     */
    int bandwidthCap;

    /**
     * Modulations defined by FrontendIsdbtModulation.
     */
    int modulationCap;

    /**
     * Code Rates defined by FrontendIsdbtCoderate.
     */
    int coderateCap;

    /**
     * Guard Interval Types defined by FrontendIsdbtGuardInterval.
     */
    int guardIntervalCap;

    /**
     * Time Interleaves defined by FrontendIsdbtTimeInterleaveMode.
     */
    int timeInterleaveCap;

    /**
     * If segment auto Supported or not.
     */
    boolean isSegmentAuto;

    /**
     * If full segment supported or not.
     */
    boolean isFullSegment;
}
