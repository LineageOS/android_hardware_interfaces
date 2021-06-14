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

/**
 * Defining signal strength type.
 */
@VintfStability
@Backing(type="int")
enum SignalMeasurementType {
    /**
     * Received Signal Strength Indication.
     * Range: -113 dBm and -51 dBm
     * Used RAN: GERAN, CDMA2000
     * Reference: 3GPP TS 27.007 section 8.5.
     */
    RSSI = 1,
    /**
     * Received Signal Code Power.
     * Range: -120 dBm to -25 dBm;
     * Used RAN: UTRAN
     * Reference: 3GPP TS 25.123, section 9.1.1.1
     */
    RSCP = 2,
    /**
     * Reference Signal Received Power.
     * Range: -140 dBm to -44 dBm;
     * Used RAN: EUTRAN
     * Reference: 3GPP TS 36.133 9.1.4
     */
    RSRP = 3,
    /**
     * Reference Signal Received Quality
     * Range: -34 dB to 3 dB;
     * Used RAN: EUTRAN
     * Reference: 3GPP TS 36.133 v12.6.0 section 9.1.7
     */
    RSRQ = 4,
    /**
     * Reference Signal Signal to Noise Ratio
     * Range: -20 dB to 30 dB;
     * Used RAN: EUTRAN
     * Note: This field is optional; how to support it can be decided by the corresponding vendor.
     * Though the response code is not enforced, vendor's implementation must ensure this interface
     * does not crash.
     */
    RSSNR = 5,
    /**
     * 5G SS reference signal received power.
     * Range: -140 dBm to -44 dBm.
     * Used RAN: NGRAN
     * Reference: 3GPP TS 38.215.
     */
    SSRSRP = 6,
    /**
     * 5G SS reference signal received quality.
     * Range: -43 dB to 20 dB.
     * Used RAN: NGRAN
     * Reference: 3GPP TS 38.215, 3GPP TS 38.133 section 10
     */
    SSRSRQ = 7,
    /**
     * 5G SS signal-to-noise and interference ratio.
     * Range: -23 dB to 40 dB
     * Used RAN: NGRAN
     * Reference: 3GPP TS 38.215 section 5.1.*, 3GPP TS 38.133 section 10.1.16.1.
     */
    SSSINR = 8,
}
