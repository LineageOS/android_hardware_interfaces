/*
 * Copyright (C) 2022 The Android Open Source Project
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

import android.hardware.wifi.WifiRateInfo;

/**
 * Per rate statistics. The rate is characterized by the combination of preamble, number
 * of spatial streams, transmission bandwidth, and modulation and coding scheme (MCS).
 */
@VintfStability
parcelable StaRateStat {
    /**
     * Wifi rate information: preamble, number of spatial streams, bandwidth, MCS, etc.
     */
    WifiRateInfo rateInfo;
    /**
     * Number of successfully transmitted data packets (ACK received).
     */
    int txMpdu;
    /**
     * Number of received data packets.
     */
    int rxMpdu;
    /**
     * Number of data packet losses (no ACK).
     */
    int mpduLost;
    /**
     * Number of data packet retries.
     */
    int retries;
}
