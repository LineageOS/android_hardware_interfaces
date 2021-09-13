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

import android.hardware.radio.Carrier;

@VintfStability
parcelable CarrierRestrictions {
    /**
     * Allowed carriers
     */
    Carrier[] allowedCarriers;
    /**
     * Explicitly excluded carriers which match allowed_carriers. Eg. allowedCarriers match mcc/mnc,
     * excludedCarriers has same mcc/mnc and gid1 is ABCD. It means except the carrier whose gid1
     * is ABCD, all carriers with the same mcc/mnc are allowed.
     */
    Carrier[] excludedCarriers;
    /**
     * Whether this is a carrier restriction with priority or not.
     * If this is false, allowedCarriersPrioritized is not applicable.
     */
    boolean priority;
    /**
     * True means that only carriers included in the allowed list and not in the excluded list
     * are permitted. Eg. allowedCarriers match mcc/mnc, excludedCarriers has same mcc/mnc and
     * gid1 is ABCD. It means except the carrier whose gid1 is ABCD, all carriers with the
     * same mcc/mnc are allowed.
     * False means that all carriers are allowed except those included in the excluded list
     * and not in the allowed list.
     */
    boolean allowedCarriersPrioritized;
}
