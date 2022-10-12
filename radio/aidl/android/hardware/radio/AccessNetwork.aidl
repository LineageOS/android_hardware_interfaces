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

@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum AccessNetwork {
    /**
     * Unknown access network
     */
    UNKNOWN = 0,
    /**
     * GSM EDGE Radio Access Network
     */
    GERAN = 1,
    /**
     * Universal Terrestrial Radio Access Network
     */
    UTRAN = 2,
    /**
     * Evolved Universal Terrestrial Radio Access Network
     */
    EUTRAN = 3,
    /**
     * CDMA 2000 network
     */
    CDMA2000 = 4,
    /**
     * Interworking Wireless LAN.
     * @deprecated should use N3ANEPS or N3AN5GS based on N3AN selection.
     */
    IWLAN = 5,
    /**
     * Next-Generation Radio Access Network (NGRAN).
     * Note NGRAN is only for standalone mode. Non-standalone mode uses AccessNetwork EUTRAN.
     */
    NGRAN = 6,
    /**
     * Non-3GPP Access Network for EPS.
     */
    N3ANEPS = 5,
    /**
     * Non-3GPP Access Network for 5GS.
     */
    N3AN5GS = 7,
}
