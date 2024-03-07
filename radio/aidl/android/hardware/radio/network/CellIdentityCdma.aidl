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

import android.hardware.radio.network.OperatorInfo;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable CellIdentityCdma {
    /**
     * Network Id 0..65535, INT_MAX if unknown
     */
    int networkId;
    /**
     * CDMA System Id 0..32767, INT_MAX if unknown
     */
    int systemId;
    /**
     * Base Station Id 0..65535, INT_MAX if unknown
     */
    int baseStationId;
    /**
     * Longitude is a decimal number as specified in 3GPP2 C.S0005-A v6.0. It is represented in
     * units of 0.25 seconds and ranges from -2592000 to 2592000, both values inclusive
     * (corresponding to a range of -180 to +180 degrees). INT_MAX if unknown
     */
    int longitude;
    /**
     * Latitude is a decimal number as specified in 3GPP2 C.S0005-A v6.0. It is represented in
     * units of 0.25 seconds and ranges from -1296000 to 1296000, both values inclusive
     * (corresponding to a range of -90 to +90 degrees). INT_MAX if unknown
     */
    int latitude;
    /**
     * OperatorInfo containing alphaLong and alphaShort
     */
    OperatorInfo operatorNames;
}
