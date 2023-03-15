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

package android.hardware.broadcastradio;

import android.hardware.broadcastradio.ProgramIdentifier;

/**
 * A set of identifiers necessary to tune to a given station.
 *
 * This can hold a combination of various identifiers, like:
 * - AM/FM frequency,
 * - HD Radio subchannel,
 * - DAB service ID.
 *
 * The type of radio technology is determined by the primary identifier - if the
 * primary identifier is for DAB, the program is DAB. However, a program of a
 * specific radio technology may have additional secondary identifiers for other
 * technologies, i.e. a satellite program may have FM fallback frequency,
 * if a station broadcasts both via satellite and FM.
 *
 * The identifiers from VENDOR_START..VENDOR_END range have limited
 * serialization capabilities: they are serialized locally, but ignored by the
 * cloud services. If a program has primary id from vendor range, it's not
 * synchronized with other devices at all.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable ProgramSelector {
    /**
     * Primary program identifier.
     *
     * This identifier uniquely identifies a station and can be used for
     * equality check.
     *
     * It can hold only a subset of identifier types, one per each
     * radio technology:
     *  - analogue AM/FM: AMFM_FREQUENCY_KHZ;
     *  - FM RDS: RDS_PI;
     *  - HD Radio: HD_STATION_ID_EXT;
     *  - DAB/DMB: DAB_SID_EXT;
     *  - Digital Radio Mondiale: DRMO_SERVICE_ID;
     *  - SiriusXM: SXM_SERVICE_ID;
     *  - vendor-specific: VENDOR_START..VENDOR_END.
     */
    ProgramIdentifier primaryId;

    /**
     * Secondary program identifiers.
     *
     * These identifiers are supplementary and can speed up tuning process,
     * but the primary ID should be sufficient (i.e. RDS PI is enough to select
     * a station from the list after a full band scan).
     *
     * Two selectors with different secondary IDs, but the same primary ID are
     * considered equal. In particular, secondary IDs array may get updated for
     * an entry on the program list (ie. when a better frequency for a given
     * station is found).
     *
     * If DAB_SID_EXT is used as primaryId, using DAB_ENSEMBLE or DAB_FREQUENCY_KHZ
     * as secondray identifiers can uniquely identify the DAB station.
     */
    ProgramIdentifier[] secondaryIds;
}
