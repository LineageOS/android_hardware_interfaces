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

import android.hardware.tv.tuner.FrontendAnalogSettings;
import android.hardware.tv.tuner.FrontendAtsc3Settings;
import android.hardware.tv.tuner.FrontendAtscSettings;
import android.hardware.tv.tuner.FrontendDtmbSettings;
import android.hardware.tv.tuner.FrontendDvbcSettings;
import android.hardware.tv.tuner.FrontendDvbsSettings;
import android.hardware.tv.tuner.FrontendDvbtSettings;
import android.hardware.tv.tuner.FrontendIptvSettings;
import android.hardware.tv.tuner.FrontendIsdbs3Settings;
import android.hardware.tv.tuner.FrontendIsdbsSettings;
import android.hardware.tv.tuner.FrontendIsdbtSettings;

/**
 * Signal Settings for Frontend.
 * @hide
 */
@VintfStability
union FrontendSettings {
    FrontendAnalogSettings analog;

    FrontendAtscSettings atsc;

    FrontendAtsc3Settings atsc3;

    FrontendDvbsSettings dvbs;

    FrontendDvbcSettings dvbc;

    FrontendDvbtSettings dvbt;

    FrontendIsdbsSettings isdbs;

    FrontendIsdbs3Settings isdbs3;

    FrontendIsdbtSettings isdbt;

    FrontendDtmbSettings dtmb;

    @nullable FrontendIptvSettings iptv;
}
