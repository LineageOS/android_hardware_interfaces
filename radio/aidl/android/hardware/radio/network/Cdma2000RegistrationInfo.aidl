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
parcelable Cdma2000RegistrationInfo {
    const int PRL_INDICATOR_NOT_REGISTERED = -1;
    const int PRL_INDICATOR_NOT_IN_PRL = 0;
    const int PRL_INDICATOR_IN_PRL = 1;
    /**
     * Concurrent services support indicator. if registered on a CDMA system.
     * false - Concurrent services not supported,
     * true - Concurrent services supported
     */
    boolean cssSupported;
    /**
     * TSB-58 Roaming Indicator if registered on a CDMA or EVDO system or -1 if not.
     * Valid values are 0-255.
     */
    int roamingIndicator;
    /**
     * Indicates whether the current system is in the PRL if registered on a CDMA or EVDO system
     * or -1 if not. 0=not in the PRL, 1=in the PRL.
     * Values are PRL_INDICATOR_
     */
    int systemIsInPrl;
    /**
     * Default Roaming Indicator from the PRL if registered on a CDMA or EVDO system or -1 if not.
     * Valid values are 0-255.
     */
    int defaultRoamingIndicator;
}
