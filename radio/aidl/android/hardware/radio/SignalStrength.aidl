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

import android.hardware.radio.CdmaSignalStrength;
import android.hardware.radio.EvdoSignalStrength;
import android.hardware.radio.GsmSignalStrength;
import android.hardware.radio.LteSignalStrength;
import android.hardware.radio.NrSignalStrength;
import android.hardware.radio.TdscdmaSignalStrength;
import android.hardware.radio.WcdmaSignalStrength;

@VintfStability
parcelable SignalStrength {
    /**
     * If GSM measurements are provided, this structure must contain valid measurements; otherwise
     * all fields should be set to INT_MAX to mark them as invalid.
     */
    GsmSignalStrength gsm;
    /**
     * If CDMA measurements are provided, this structure must contain valid measurements; otherwise
     * all fields should be set to INT_MAX to mark them as invalid.
     */
    CdmaSignalStrength cdma;
    /**
     * If EvDO measurements are provided, this structure must contain valid measurements; otherwise
     * all fields should be set to INT_MAX to mark them as invalid.
     */
    EvdoSignalStrength evdo;
    /**
     * If LTE measurements are provided, this structure must contain valid measurements; otherwise
     * all fields should be set to INT_MAX to mark them as invalid.
     */
    LteSignalStrength lte;
    /**
     * If TD-SCDMA measurements are provided, this structure must contain valid measurements;
     * otherwise all fields should be set to INT_MAX to mark them as invalid.
     */
    TdscdmaSignalStrength tdscdma;
    /**
     * If WCDMA measurements are provided, this structure must contain valid measurements; otherwise
     * all fields should be set to INT_MAX to mark them as invalid.
     */
    WcdmaSignalStrength wcdma;
    /**
     * If NR 5G measurements are provided, this structure must contain valid measurements; otherwise
     * all fields should be set to INT_MAX to mark them as invalid.
     */
    NrSignalStrength nr;
}
