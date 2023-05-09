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

import android.hardware.radio.AccessNetwork;

/**
 * Contains the threshold values of each signal measurement type.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable SignalThresholdInfo {
    /**
     * Received Signal Strength Indication.
     * Range: -113 dBm and -51 dBm
     * Used RAN: GERAN, CDMA2000
     * Reference: 3GPP TS 27.007 section 8.5.
     */
    const int SIGNAL_MEASUREMENT_TYPE_RSSI = 1;
    /**
     * Received Signal Code Power.
     * Range: -120 dBm to -25 dBm;
     * Used RAN: UTRAN
     * Reference: 3GPP TS 25.123, section 9.1.1.1
     */
    const int SIGNAL_MEASUREMENT_TYPE_RSCP = 2;
    /**
     * Reference Signal Received Power.
     * Range: -140 dBm to -44 dBm;
     * Used RAN: EUTRAN
     * Reference: 3GPP TS 36.133 9.1.4
     */
    const int SIGNAL_MEASUREMENT_TYPE_RSRP = 3;
    /**
     * Reference Signal Received Quality
     * Range: -34 dB to 3 dB;
     * Used RAN: EUTRAN
     * Reference: 3GPP TS 36.133 v12.6.0 section 9.1.7
     */
    const int SIGNAL_MEASUREMENT_TYPE_RSRQ = 4;
    /**
     * Reference Signal Signal to Noise Ratio
     * Range: -20 dB to 30 dB;
     * Used RAN: EUTRAN
     * Note: This field is optional; how to support it can be decided by the corresponding vendor.
     * Though the response code is not enforced, vendor's implementation must ensure this interface
     * does not crash.
     */
    const int SIGNAL_MEASUREMENT_TYPE_RSSNR = 5;
    /**
     * 5G SS reference signal received power.
     * Range: -140 dBm to -44 dBm.
     * Used RAN: NGRAN
     * Reference: 3GPP TS 38.215.
     */
    const int SIGNAL_MEASUREMENT_TYPE_SSRSRP = 6;
    /**
     * 5G SS reference signal received quality.
     * Range: -43 dB to 20 dB.
     * Used RAN: NGRAN
     * Reference: 3GPP TS 38.215, 3GPP TS 38.133 section 10
     */
    const int SIGNAL_MEASUREMENT_TYPE_SSRSRQ = 7;
    /**
     * 5G SS signal-to-noise and interference ratio.
     * Range: -23 dB to 40 dB
     * Used RAN: NGRAN
     * Reference: 3GPP TS 38.215 section 5.1.*, 3GPP TS 38.133 section 10.1.16.1.
     */
    const int SIGNAL_MEASUREMENT_TYPE_SSSINR = 8;
    /**
     * EcNo value
     * Range: -24 dBm to 1 dBm.
     * Used RAN: UTRAN
     * Reference: 3GPP TS 25.215 5.1.5
     */
    const int SIGNAL_MEASUREMENT_TYPE_ECNO = 9;
    /**
     * Signal Measurement Type
     * Values are SIGNAL_MEASUREMENT_TYPE_
     */
    int signalMeasurement;
    /**
     * A hysteresis time in milliseconds for current signal measurement type to prevent flapping.
     * A value of 0 disables hysteresis.
     */
    int hysteresisMs;
    /**
     * An interval in dB for current signal measurement type defining the required magnitude change
     * between reports. This must be smaller than the smallest threshold delta. An interval value of
     * 0 disables hysteresis.
     */
    int hysteresisDb;
    /**
     * List of threshold values for current signal measurement type. Range and unit must reference
     * specific SignalMeasurementType. The threshold values for which to apply criteria. A vector
     * size of 0 disables the use of thresholds for reporting.
     */
    int[] thresholds;
    /**
     * Indicates whether the reporting criteria of the corresponding measurement is enabled
     * (true) or disabled (false). If enabled, modem must trigger the report based on the criteria.
     * If disabled, modem must not trigger the report based on the criteria.
     */
    boolean isEnabled;
    /**
     * The Radio Access Network for current threshold info.
     */
    AccessNetwork ran;
}
