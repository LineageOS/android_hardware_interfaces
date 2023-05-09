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

package android.hardware.radio.voice;

/**
 * User-to-User Signaling Information defined in 3GPP 23.087 v8.0
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable UusInfo {
    /**
     * User specified protocol
     */
    const int UUS_DCS_USP = 0;
    /**
     * OSI higher layer protocol
     */
    const int UUS_DCS_OSIHLP = 1;
    /**
     * X.244
     */
    const int UUS_DCS_X244 = 2;
    /**
     * Reserved for system management
     */
    const int UUS_DCS_RMCF = 3;
    /**
     * IA5 characters
     */
    const int UUS_DCS_IA5C = 4;

    const int UUS_TYPE_TYPE1_IMPLICIT = 0;
    const int UUS_TYPE_TYPE1_REQUIRED = 1;
    const int UUS_TYPE_TYPE1_NOT_REQUIRED = 2;
    const int UUS_TYPE_TYPE2_REQUIRED = 3;
    const int UUS_TYPE_TYPE2_NOT_REQUIRED = 4;
    const int UUS_TYPE_TYPE3_REQUIRED = 5;
    const int UUS_TYPE_TYPE3_NOT_REQUIRED = 6;

    /**
     * User-to-User Signaling Information activation types derived from 3GPP 23.087 v8.0
     * Values are UUS_TYPE_
     */
    int uusType;
    /**
     * User-to-User Signaling Information data coding schemes. Possible values for Octet 3 (Protocol
     * Discriminator field) in the UUIE. The values have been specified in section 10.5.4.25 of
     * 3GPP TS 24.008
     * Values are UUS_DCS_
     */
    int uusDcs;
    /**
     * UUS data
     */
    String uusData;
}
