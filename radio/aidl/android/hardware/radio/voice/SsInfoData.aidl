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

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable SsInfoData {
    const int SS_INFO_MAX = 4;
    /**
     * This is the response data for all of the SS GET/SET Radio requests.
     * E.g. IRadioVoice.getClir() returns two ints, so first two values of ssInfo[] will be used for
     * response if serviceType is SS_CLIR and requestType is SS_INTERROGATION.
     * Max size = SS_INFO_MAX
     */
    int[] ssInfo;
}
