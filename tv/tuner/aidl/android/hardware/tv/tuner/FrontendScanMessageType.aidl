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
 * Scan Message Type for Frontend.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendScanMessageType {
    /**
     * Scan locked the signal.
     */
    LOCKED,

    /**
     * Scan stopped.
     */
    END,

    /**
     * Scan progress report.
     */
    PROGRESS_PERCENT,

    /**
     * Locked frequency report.
     */
    FREQUENCY,

    /**
     * Locked symbol rate.
     */
    SYMBOL_RATE,

    /**
     * Locked HIERARCHY for DVBT2 frontend.
     */
    HIERARCHY,

    ANALOG_TYPE,

    /**
     * Locked Plp Ids for DVBT2 frontend.
     */

    PLP_IDS,

    /**
     * Locked group Ids for DVBT2 frontend.
     */
    GROUP_IDS,

    /**
     * Stream Ids.
     */
    INPUT_STREAM_IDS,

    /**
     * Locked signal standard.
     */
    STANDARD,

    /**
     * PLP status in a tuned frequency band for ATSC3 frontend.
     */
    ATSC3_PLP_INFO,

    MODULATION,

    DVBC_ANNEX,

    HIGH_PRIORITY,

    /**
     * DVB-T CELL ID.
     */
    DVBT_CELL_IDS,

}
