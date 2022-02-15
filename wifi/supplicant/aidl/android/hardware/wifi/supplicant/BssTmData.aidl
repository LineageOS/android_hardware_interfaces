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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.supplicant.BssTmDataFlagsMask;
import android.hardware.wifi.supplicant.BssTmStatusCode;
import android.hardware.wifi.supplicant.MboCellularDataConnectionPrefValue;
import android.hardware.wifi.supplicant.MboTransitionReasonCode;

/**
 * Data retrieved from received BSS transition management request frame.
 */
@VintfStability
parcelable BssTmData {
    /*
     * Status code filled in BSS transition management response frame
     */
    BssTmStatusCode status;
    /*
     * Bitmask of BssTmDataFlagsMask
     */
    BssTmDataFlagsMask flags;
    /*
     * Duration for which STA shouldn't try to re-associate.
     */
    int assocRetryDelayMs;
    /*
     * Reason for BSS transition request.
     */
    MboTransitionReasonCode mboTransitionReason;
    /*
     * Cellular Data Connection preference value.
     */
    MboCellularDataConnectionPrefValue mboCellPreference;
}
