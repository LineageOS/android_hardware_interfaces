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

@VintfStability
@Backing(type="int")
enum PreferredNetworkType {
    /**
     * GSM/WCDMA (WCDMA preferred)
     */
    GSM_WCDMA,
    /**
     * GSM only
     */
    GSM_ONLY,
    /**
     * WCDMA
     */
    WCDMA,
    /**
     * GSM/WCDMA (auto mode, according to PRL)
     */
    GSM_WCDMA_AUTO,
    /**
     * CDMA and EvDo (auto mode, according to PRL)
     */
    CDMA_EVDO_AUTO,
    /**
     * CDMA only
     */
    CDMA_ONLY,
    /**
     * EvDo only
     */
    EVDO_ONLY,
    /**
     * GSM/WCDMA, CDMA, and EvDo (auto mode, according to PRL)
     */
    GSM_WCDMA_CDMA_EVDO_AUTO,
    /**
     * LTE, CDMA and EvDo
     */
    LTE_CDMA_EVDO,
    /**
     * LTE, GSM/WCDMA
     */
    LTE_GSM_WCDMA,
    /**
     * LTE, CDMA, EvDo, GSM/WCDMA
     */
    LTE_CMDA_EVDO_GSM_WCDMA,
    /**
     * LTE only
     */
    LTE_ONLY,
    /**
     * LTE/WCDMA only
     */
    LTE_WCDMA,
    /**
     * TD-SCDMA only
     */
    TD_SCDMA_ONLY,
    /**
     * TD-SCDMA and WCDMA
     */
    TD_SCDMA_WCDMA,
    /**
     * TD-SCDMA and LTE
     */
    TD_SCDMA_LTE,
    /**
     * TD-SCDMA and GSM
     */
    TD_SCDMA_GSM,
    /**
     * TD-SCDMA,GSM and LTE
     */
    TD_SCDMA_GSM_LTE,
    /**
     * TD-SCDMA, GSM/WCDMA
     */
    TD_SCDMA_GSM_WCDMA,
    /**
     * TD-SCDMA, WCDMA and LTE
     */
    TD_SCDMA_WCDMA_LTE,
    /**
     * TD-SCDMA, GSM/WCDMA and LTE
     */
    TD_SCDMA_GSM_WCDMA_LTE,
    /**
     * TD-SCDMA, GSM/WCDMA, CDMA and EvDo
     */
    TD_SCDMA_GSM_WCDMA_CDMA_EVDO_AUTO,
    /**
     * TD-SCDMA, LTE, CDMA, EvDo GSM/WCDMA
     */
    TD_SCDMA_LTE_CDMA_EVDO_GSM_WCDMA,
}
