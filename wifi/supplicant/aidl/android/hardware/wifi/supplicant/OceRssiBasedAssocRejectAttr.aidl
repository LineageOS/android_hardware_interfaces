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

/**
 * OceRssiBasedAssocRejectAttr is extracted from (Re-)Association response
 * frame from an OCE AP to indicate that the AP has rejected the
 * (Re-)Association request on the basis of insufficient RSSI.
 * Refer OCE spec v1.0 section 4.2.2 Table 7.
 */
@VintfStability
parcelable OceRssiBasedAssocRejectAttr {
    /*
     * Delta RSSI - The difference in dB between the minimum RSSI at which
     * the AP would accept a (Re-)Association request from the STA before
     * Retry Delay expires and the AP's measurement of the RSSI at which the
     * (Re-)Association request was received.
     */
    int deltaRssi;
    /*
     * Retry Delay - The time period in seconds for which the AP will not
     * accept any subsequent (Re-)Association requests from the STA, unless
     * the received RSSI has improved by Delta RSSI.
     */
    int retryDelayS;
}
