/*
 * Copyright (C) 2023 The Android Open Source Project
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

import android.hardware.radio.sim.Plmn;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable CarrierInfo {
    /**
     * MCC (Mobile Country Code) of Carrier. Wild char is either '*' or '?'.
     */
    String mcc;

    /**
     * MNC (Mobile Network Code) of the Carrier. Wild char is either '*' or '?'.
     */
    String mnc;
    /**
     * Service Provider Name(SPN) of the SIM card of the Carrier.
     */
    @nullable
    String spn;
    /**
     * GID1 value of the SIM card of the Carrier.
     */
    @nullable
    String gid1;
    /**
     * GID2 value of the SIM card of the Carrier.
     */
    @nullable
    String gid2;

    /**
     * IMSI (International Mobile Subscriber Identity) prefix. Wild char is '*'.
     */
    @nullable
    String imsiPrefix;
    /**
     * Equivalent HPLMN of the SIM card of the Carrier.
     */
    @nullable
    List<Plmn> ephlmn;
    /**
     * ICCID (Integrated Circuit Card Identification) of the SIM card.
     */
    @nullable
    String iccid;
    /**
     * IMPI (IMS Private Identity) of the SIM card of the Carrier.
     */
    @nullable
    String impi;
}