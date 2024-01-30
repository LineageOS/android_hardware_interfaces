/*
 * Copyright (C) 2022 The Android Open Source Project
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
@Backing(type="int")
@JavaDerive(toString=true)
enum EmergencyScanType {
    /**
     * Scan Type No Preference, indicates that the modem can scan for emergency
     * service as per modem’s implementation.
     */
    NO_PREFERENCE = 0,

    /**
     * Scan Type limited, indicates that the modem will scan for
     * emergency service in limited service mode.
     */
    LIMITED_SERVICE = 1,

    /**
     * Scan Type Full Service, indicates that the modem will scan for
     * emergency service in Full service mode.
     */
    FULL_SERVICE = 2,
}
