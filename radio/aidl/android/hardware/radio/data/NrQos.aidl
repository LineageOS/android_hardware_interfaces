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

package android.hardware.radio.data;

import android.hardware.radio.data.QosBandwidth;

/**
 * 5G Quality of Service parameters as per 3gpp spec 24.501 sec 9.11.4.12
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable NrQos {
    const byte FLOW_ID_RANGE_MIN = 1;
    const byte FLOW_ID_RANGE_MAX = 63;

    const int AVERAGING_WINDOW_UNKNOWN = -1;

    /**
     * 5G QOS Identifier (5QI), see 3GPP TS 24.501 and 23.501. The allowed values are standard
     * values (1-9, 65-68, 69-70, 75, 79-80, 82-85) defined in the spec and operator specific values
     * in the range 128-254.
     */
    int fiveQi;
    QosBandwidth downlink;
    QosBandwidth uplink;
    /**
     * QOS flow identifier of the QOS flow description in the range
     * (FLOW_ID_RANGE_MIN, FLOW_ID_RANGE_MAX).
     */
    byte qfi;
    /**
     * @deprecated use averagingWindowMillis;
     */
    char averagingWindowMs;
    /**
     * The duration over which flow rates are calculated.
     */
    int averagingWindowMillis = AVERAGING_WINDOW_UNKNOWN;
}
