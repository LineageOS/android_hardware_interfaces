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

package android.hardware.radio;

import android.hardware.radio.SimRefreshType;

@VintfStability
parcelable SimRefreshResult {
    SimRefreshType type;
    /**
     * EFID of the updated file if the result is SIM_FILE_UPDATE or 0 for any other result.
     */
    int efId;
    /**
     * AID(application ID) of the card application. See ETSI 102.221 8.1 and 101.220 4.
     * For SIM_FILE_UPDATE result it must be set to AID of application in which updated EF resides
     * or it must be empty string if EF is outside of an application. For SIM_INIT result this field
     * is set to AID of application that caused REFRESH. For SIM_RESET result it is empty string.
     */
    String aid;
}
