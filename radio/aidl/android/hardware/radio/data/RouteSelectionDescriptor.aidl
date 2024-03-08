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

import android.hardware.radio.data.PdpProtocolType;
import android.hardware.radio.data.SliceInfo;

/**
 * This struct represents a single route selection descriptor as defined in 3GPP TS 24.526.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable RouteSelectionDescriptor {
    const byte SSC_MODE_UNKNOWN = -1;
    const byte SSC_MODE_1 = 1;
    const byte SSC_MODE_2 = 2;
    const byte SSC_MODE_3 = 3;

    /**
     * Precedence value in the range of 0 to 255. Higher value has lower precedence.
     */
    byte precedence;
    /**
     * Valid values are IP, IPV6, IPV4V6, and UNKNOWN.
     */
    PdpProtocolType sessionType;
    /**
     * Session and service continuity mode as defined in 3GPP TS 23.501.
     * Valid values are SSC_MODE_
     */
    byte sscMode;
    /**
     * There can be 0 or more SliceInfo specified in a route descriptor.
     */
    SliceInfo[] sliceInfo;
    /**
     * DNN stands for Data Network Name and represents an APN as defined in 3GPP TS 23.003.
     * There can be 0 or more DNNs specified in a route descriptor.
     */
    String[] dnn;
}
