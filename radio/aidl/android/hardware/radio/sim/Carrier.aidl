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

package android.hardware.radio.sim;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable Carrier {
    /**
     * Apply to all carrier with the same mcc/mnc
     */
    const int MATCH_TYPE_ALL = 0;
    /**
     * Use SPN and mcc/mnc to identify the carrier
     */
    const int MATCH_TYPE_SPN = 1;
    /**
     * Use IMSI prefix and mcc/mnc to identify the carrier
     */
    const int MATCH_TYPE_IMSI_PREFIX = 2;
    /**
     * Use GID1 and mcc/mnc to identify the carrier
     */
    const int MATCH_TYPE_GID1 = 3;
    /**
     * Use GID2 and mcc/mnc to identify the carrier
     */
    const int MATCH_TYPE_GID2 = 4;

    String mcc;
    String mnc;
    /**
     * Specify match type for the carrier. If it’s MATCH_TYPE_ALL, matchData is empty string;
     * otherwise, matchData is the value for the match type.
     * Values are MATCH_TYPE_
     */
    int matchType;
    String matchData;
}
