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

package android.hardware.radio.modem;

import android.hardware.radio.modem.ActivityStatsTechSpecificInfo;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable ActivityStatsInfo {
    /**
     * Total time (in ms) when modem is in a low power or sleep state
     */
    int sleepModeTimeMs;
    /**
     * Total time (in ms) when modem is awake but neither the transmitter nor receiver are
     * active/awake
     */
    int idleModeTimeMs;
    /**
     * Technology specific activity stats info.
     * List of the activity stats for each RATs (2G, 3G, 4G and 5G) and frequency ranges (HIGH for
     * sub6 and MMWAVE) in case of 5G. In case implementation doesn't have RAT specific activity
     * stats then send only one activity stats info with RAT unknown.
     */
    ActivityStatsTechSpecificInfo[] techSpecificInfo;
}
