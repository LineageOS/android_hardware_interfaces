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

import android.hardware.tv.tuner.FrontendAnalogType;
import android.hardware.tv.tuner.FrontendDvbcAnnex;
import android.hardware.tv.tuner.FrontendDvbtHierarchy;
import android.hardware.tv.tuner.FrontendModulation;
import android.hardware.tv.tuner.FrontendScanAtsc3PlpInfo;
import android.hardware.tv.tuner.FrontendScanMessageStandard;

/**
 * Scan Message for Frontend.
 * @hide
 */
@VintfStability
union FrontendScanMessage {
    boolean isLocked;

    boolean isEnd;

    /**
     * scan progress percent (0..100)
     */
    int progressPercent;

    /**
     * Signal frequencies in Hertz
     */
    long[] frequencies;

    /**
     * Symbols per second
     */
    int[] symbolRates;

    FrontendDvbtHierarchy hierarchy;

    FrontendAnalogType analogType;

    int[] plpIds;

    int[] groupIds;

    int[] inputStreamIds;

    FrontendScanMessageStandard std;

    /**
     * A list of PLP status in a tuned frequency band for ATSC3 frontend.
     */
    FrontendScanAtsc3PlpInfo[] atsc3PlpInfos;

    FrontendModulation modulation;

    FrontendDvbcAnnex annex;

    boolean isHighPriority;

    /**
     * DVB-T Cell Ids.
     */
    int[] dvbtCellIds;

}
