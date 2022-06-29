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

import android.hardware.tv.tuner.FrontendInnerFec;

/**
 * Code Rate for DVBS.
 * @hide
 */
@VintfStability
parcelable FrontendDvbsCodeRate {
    FrontendInnerFec fec = FrontendInnerFec.FEC_UNDEFINED;

    boolean isLinear;

    /**
     * true if enable short frame
     */
    boolean isShortFrames;

    /**
     * bits number in 1000 symbol. 0 if use the default.
     */
    int bitsPer1000Symbol;
}
