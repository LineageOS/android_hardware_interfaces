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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.common.OuiKeyedData;
import android.hardware.wifi.supplicant.P2pScanType;

/**
 * Request parameters used for |ISupplicantP2pIface.findWithParams|
 */
@VintfStability
parcelable P2pDiscoveryInfo {
    /**
     * P2P scan type.
     */
    P2pScanType scanType;

    /**
     * Frequency to scan in MHz. Only valid the scan type is |P2pScanType.SPECIFIC_FREQ|
     */
    int frequencyMhz;

    /**
     * Max time, in seconds, to be spent in performing discovery.
     * Set to 0 to indefinitely continue discovery until an explicit
     * |stopFind| is sent.
     */
    int timeoutInSec;

    /**
     * Optional vendor-specific parameters. Null value indicates
     * that no vendor data is provided.
     */
    @nullable OuiKeyedData[] vendorData;
}
