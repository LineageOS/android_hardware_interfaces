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

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable OperatorInfo {
    const int STATUS_UNKNOWN = 0;
    const int STATUS_AVAILABLE = 1;
    const int STATUS_CURRENT = 2;
    const int STATUS_FORBIDDEN = 3;

    /**
     * Long alpha ONS or EONS
     */
    String alphaLong;
    /**
     * Short alpha ONS or EONS
     */
    String alphaShort;
    /**
     * 5 or 6 digit numeric code (MCC + MNC)
     */
    String operatorNumeric;
    /**
     * Values are STATUS_
     */
    int status;
}
