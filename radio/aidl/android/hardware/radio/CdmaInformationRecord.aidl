/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.radio;

import android.hardware.radio.CdmaDisplayInfoRecord;
import android.hardware.radio.CdmaInfoRecName;
import android.hardware.radio.CdmaLineControlInfoRecord;
import android.hardware.radio.CdmaNumberInfoRecord;
import android.hardware.radio.CdmaRedirectingNumberInfoRecord;
import android.hardware.radio.CdmaSignalInfoRecord;
import android.hardware.radio.CdmaT53AudioControlInfoRecord;
import android.hardware.radio.CdmaT53ClirInfoRecord;

@VintfStability
parcelable CdmaInformationRecord {
    /**
     * Based on CdmaInfoRecName, only one of the below vectors must have size = 1.
     * All other vectors must have size 0.
     */
    CdmaInfoRecName name;
    /**
     * Display and extended display info rec
     */
    CdmaDisplayInfoRecord[] display;
    /**
     * Called party number, calling party number, connected number info rec
     */
    CdmaNumberInfoRecord[] number;
    /**
     * Signal info rec
     */
    CdmaSignalInfoRecord[] signal;
    /**
     * Redirecting number info rec
     */
    CdmaRedirectingNumberInfoRecord[] redir;
    /**
     * Line control info rec
     */
    CdmaLineControlInfoRecord[] lineCtrl;
    /**
     * T53 CLIR info rec
     */
    CdmaT53ClirInfoRecord[] clir;
    /**
     * T53 Audio Control info rec
     */
    CdmaT53AudioControlInfoRecord[] audioCtrl;
}
