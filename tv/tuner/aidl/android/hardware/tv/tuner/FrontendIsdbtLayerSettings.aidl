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

import android.hardware.tv.tuner.FrontendIsdbtCoderate;
import android.hardware.tv.tuner.FrontendIsdbtModulation;
import android.hardware.tv.tuner.FrontendIsdbtTimeInterleaveMode;

/**
 * Layer Settings for ISDB-T Frontend.
 * @hide
 */
@VintfStability
parcelable FrontendIsdbtLayerSettings {
    FrontendIsdbtModulation modulation = FrontendIsdbtModulation.UNDEFINED;

    FrontendIsdbtCoderate coderate = FrontendIsdbtCoderate.UNDEFINED;

    FrontendIsdbtTimeInterleaveMode timeInterleave = FrontendIsdbtTimeInterleaveMode.UNDEFINED;

    /**
     * 0 ~ 13 and 0xFF(Auto)
     */
    int numOfSegment;
}
