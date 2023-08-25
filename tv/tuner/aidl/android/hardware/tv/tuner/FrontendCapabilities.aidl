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

import android.hardware.tv.tuner.FrontendAnalogCapabilities;
import android.hardware.tv.tuner.FrontendAtsc3Capabilities;
import android.hardware.tv.tuner.FrontendAtscCapabilities;
import android.hardware.tv.tuner.FrontendDtmbCapabilities;
import android.hardware.tv.tuner.FrontendDvbcCapabilities;
import android.hardware.tv.tuner.FrontendDvbsCapabilities;
import android.hardware.tv.tuner.FrontendDvbtCapabilities;
import android.hardware.tv.tuner.FrontendIptvCapabilities;
import android.hardware.tv.tuner.FrontendIsdbs3Capabilities;
import android.hardware.tv.tuner.FrontendIsdbsCapabilities;
import android.hardware.tv.tuner.FrontendIsdbtCapabilities;

/**
 * @hide
 */
@VintfStability
union FrontendCapabilities {
    FrontendAnalogCapabilities analogCaps;

    FrontendAtscCapabilities atscCaps;

    FrontendAtsc3Capabilities atsc3Caps;

    FrontendDtmbCapabilities dtmbCaps;

    FrontendDvbsCapabilities dvbsCaps;

    FrontendDvbcCapabilities dvbcCaps;

    FrontendDvbtCapabilities dvbtCaps;

    FrontendIsdbsCapabilities isdbsCaps;

    FrontendIsdbs3Capabilities isdbs3Caps;

    FrontendIsdbtCapabilities isdbtCaps;

    @nullable FrontendIptvCapabilities iptvCaps;
}
