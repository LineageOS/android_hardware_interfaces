/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.gnss.gnss_assistance;

import android.hardware.gnss.GnssConstellationType;

/*
 * Contains the GNSS-GNSS system time offset between the GNSS system time.
 * This is defined in RINEX 3.05 "TIME SYSTEM CORR" in table A5.
 *
 * @hide
 */
@VintfStability
parcelable TimeModel {
    /*
     * Model represents parameters to convert from current GNSS to GNSS system
     * time indicated by toGnss.
     */
    GnssConstellationType toGnss;

    /** Bias coefficient of GNSS time scale relative to GNSS time scale in seconds. */
    double a0;

    /** Drift coefficient of GNSS time scale relative to GNSS time scale in seconds per second. */
    double a1;

    /** Reference GNSS time of week in seconds. */
    int timeOfWeek;

    /** Reference GNSS week number. */
    int weekNumber;
}
