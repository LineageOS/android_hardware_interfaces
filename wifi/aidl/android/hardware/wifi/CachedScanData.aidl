/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.wifi;

import android.hardware.wifi.CachedScanResult;

/**
 * Scan data cached in Wifi firmware
 */
@VintfStability
parcelable CachedScanData {
    /**
     * List of scanned frequencies in MHz.
     */
    int[] scannedFrequenciesMhz;

    /**
     * List of scan results.
     */
    CachedScanResult[] cachedScanResults;
}
