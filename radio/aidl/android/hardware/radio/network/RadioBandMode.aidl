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

package android.hardware.radio.network;

/** @hide */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum RadioBandMode {
    /**
     * "Unspecified" (selected by baseband automatically)
     */
    BAND_MODE_UNSPECIFIED,
    /**
     * "EURO band" (GSM-900 / DCS-1800 / WCDMA-IMT-2000)
     */
    BAND_MODE_EURO,
    /**
     * "US band" (GSM-850 / PCS-1900 / WCDMA-850 / WCDMA-PCS-1900)
     */
    BAND_MODE_USA,
    /**
     * "JPN band" (WCDMA-800 / WCDMA-IMT-2000)
     */
    BAND_MODE_JPN,
    /**
     * "AUS band" (GSM-900 / DCS-1800 / WCDMA-850 / WCDMA-IMT-2000)
     */
    BAND_MODE_AUS,
    /**
     * "AUS band 2" (GSM-900 / DCS-1800 / WCDMA-850)
     */
    BAND_MODE_AUS_2,
    /**
     * "Cellular" (800-MHz Band)
     */
    BAND_MODE_CELL_800,
    /**
     * "PCS" (1900-MHz Band)
     */
    BAND_MODE_PCS,
    /**
     * "Band Class 3" (JTACS Band)
     */
    BAND_MODE_JTACS,
    /**
     * "Band Class 4" (Korean PCS Band)
     */
    BAND_MODE_KOREA_PCS,
    /**
     * "Band Class 5" (450-MHz Band)
     */
    BAND_MODE_5_450M,
    /**
     * "Band Class 6" (2-GMHz IMT2000 Band)
     */
    BAND_MODE_IMT2000,
    /**
     * "Band Class 7" (Upper 700-MHz Band)
     */
    BAND_MODE_7_700M_2,
    /**
     * "Band Class 8" (1800-MHz Band)
     */
    BAND_MODE_8_1800M,
    /**
     * "Band Class 9" (900-MHz Band)
     */
    BAND_MODE_9_900M,
    /**
     * "Band Class 10" (Secondary 800-MHz Band)
     */
    BAND_MODE_10_800M_2,
    /**
     * "Band Class 11" (400-MHz European PAMR Band)
     */
    BAND_MODE_EURO_PAMR_400M,
    /**
     * "Band Class 15" (AWS Band)
     */
    BAND_MODE_AWS,
    /**
     * "Band Class 16" (US 2.5-GHz Band)
     */
    BAND_MODE_USA_2500M,
}
