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
 * Capabilities for ATSC3 Frontend.
 * @hide
 */
@VintfStability
parcelable FrontendAtsc3Capabilities {
    /**
     * Bandwidth capabilities defined by FrontendAtsc3Bandwidth.
     */
    int bandwidthCap;

    /**
     * Modulation capabilities defined by FrontendAtsc3Modulation.
     */
    int modulationCap;

    /**
     * TimeInterleaveMode capabilities defined by FrontendAtsc3TimeInterleaveMode.
     */
    int timeInterleaveModeCap;

    /**
     * CodeRate capabilities defined by FrontendAtsc3CodeRate.
     */
    int codeRateCap;

    /**
     * FEC capabilities defined by FrontendAtsc3Fec.
     */
    int fecCap;

    /**
     * Demodulator Output Format capabilities FrontendAtsc3DemodOutputFormat.
     */
    byte demodOutputFormatCap;
}
