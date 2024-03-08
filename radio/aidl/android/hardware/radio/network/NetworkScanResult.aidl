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

import android.hardware.radio.RadioError;
import android.hardware.radio.network.CellInfo;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable NetworkScanResult {
    /**
     * The result contains a part of the scan results.
     */
    const int SCAN_STATUS_PARTIAL = 1;
    /**
     * The result contains the last part of the scan results.
     */
    const int SCAN_STATUS_COMPLETE = 2;

    /**
     * The status of the scan.
     * Values are SCAN_STATUS_
     */
    int status;
    /**
     * The error code of the incremental result.
     */
    RadioError error;
    /**
     * List of network information as CellInfo.
     */
    CellInfo[] networkInfos;
}
