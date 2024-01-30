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

import android.hardware.radio.sim.Carrier;
import android.hardware.radio.sim.CarrierInfo;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable CarrierRestrictions {
    @VintfStability
    @Backing(type="int")
    /** This enum defines the carrier restriction status values */
    enum CarrierRestrictionStatus {
        /**
         * Carrier restriction status value is unknown, used in cases where modem is dependent on
         * external module to know about the lock status and the module hasn’t yet provided the lock
         * status. For example, when the lock status is maintained on a cloud server and device has
         * just booted after out of box and not yet connected to the internet.
         */
        UNKNOWN = 0,
        /** There is no carrier restriction on the device */
        NOT_RESTRICTED = 1,
        /** The device is restricted to a carrier */
        RESTRICTED = 2,
    }
    /**
     * Allowed carriers
     * @deprecated use @List<CarrierInfo> allowedCarrierInfoList
     */
    Carrier[] allowedCarriers;
    /**
     * Explicitly excluded carriers which match allowed_carriers. Eg. allowedCarriers match mcc/mnc,
     * excludedCarriers has same mcc/mnc and gid1 is ABCD. It means except the carrier whose gid1
     * is ABCD, all carriers with the same mcc/mnc are allowed.
     * @deprecated use @List<CarrierInfo> excludedCarrierInfoList
     */
    Carrier[] excludedCarriers;
    /**
     * True means that only carriers included in the allowed list and not in the excluded list
     * are permitted. Eg. allowedCarriers match mcc/mnc, excludedCarriers has same mcc/mnc and
     * gid1 is ABCD. It means except the carrier whose gid1 is ABCD, all carriers with the
     * same mcc/mnc are allowed.
     * False means that all carriers are allowed except those included in the excluded list
     * and not in the allowed list.
     */
    boolean allowedCarriersPrioritized;
    /** Current restriction status as defined in CarrierRestrictionStatus enum */
    CarrierRestrictionStatus status;

    /**  Allowed carriers. */
    CarrierInfo[] allowedCarrierInfoList = {};

    /**
     * Explicitly excluded carriers which match allowed_carriers. Eg. allowedCarriers match mcc/mnc,
     * excludedCarriers has same mcc/mnc and gid1 is ABCD. It means except the carrier whose gid1
     * is ABCD, all carriers with the same mcc/mnc are allowed.
     */
    CarrierInfo[]  excludedCarrierInfoList = {};
}