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
parcelable SimRefreshResult {
    /**
     * A file on SIM has been updated.
     */
    const int TYPE_SIM_FILE_UPDATE = 0;
    /**
     * SIM initialized. All files should be re-read.
     */
    const int TYPE_SIM_INIT = 1;
    /**
     * SIM reset. SIM power required, SIM may be locked and all files must be re-read.
     */
    const int TYPE_SIM_RESET = 2;

    /**
     * Values are TYPE_SIM_
     */
    int type;
    /**
     * EFID of the updated file if the result is SIM_FILE_UPDATE or 0 for any other result.
     */
    int efId;
    /**
     * AID (application ID) of the card application. See ETSI 102.221 8.1 and 101.220 4.
     * For TYPE_SIM_FILE_UPDATE result, it must be set to AID of application in which updated EF
     * resides or it must be empty string if EF is outside of an application. For TYPE_SIM_INIT
     * result, this field is set to AID of application that caused REFRESH. For TYPE_SIM_RESET
     * result, it is empty string.
     */
    String aid;
}
