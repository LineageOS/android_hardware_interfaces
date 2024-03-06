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

/** @hide */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum AccessNetwork {
    /**
     * Unknown access network
     */
    UNKNOWN,
    /**
     * GSM EDGE Radio Access Network
     */
    GERAN,
    /**
     * Universal Terrestrial Radio Access Network
     */
    UTRAN,
    /**
     * Evolved Universal Terrestrial Radio Access Network
     */
    EUTRAN,
    /**
     * CDMA 2000 network
     */
    CDMA2000,
    /**
     * Interworking Wireless LAN
     */
    IWLAN,
    /**
     * Next-Generation Radio Access Network (NGRAN).
     * Note NGRAN is only for standalone mode. Non-standalone mode uses AccessNetwork EUTRAN.
     */
    NGRAN,
}
