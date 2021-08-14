/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.gnss;

import android.hardware.gnss.BlocklistedSource;

/**
 * Extended interface for GNSS Configuration support.
 */
@VintfStability
interface IGnssConfiguration {

    /** SUPL mode bitmask for Mobile Station Based. */
    const int SUPL_MODE_MSB = 0x01;

    /** SUPL mode bitmask for Mobile Station Assisted. */
    const int SUPL_MODE_MSA = 0x02;

    /** LPP profile settings bitmask for enabling LTE Positioning Protocol User Plane. */
    const int LPP_PROFILE_USER_PLANE = 0x01;

    /** LPP profile settings bitmask for enabling LTE Positioning Protocol Control Plane. */
    const int LPP_PROFILE_CONTROL_PLANE = 0x02;

    /** A-Glonass positioning protocol bitmask for Radio Resource Control (RRC) Control Plane. */
    const int GLONASS_POS_PROTOCOL_RRC_CPLANE = 0x01;

    /** A-Glonass positioning protocol bitmask for Radio Resource Location User Plane. */
    const int GLONASS_POS_PROTOCOL_RRLP_UPLANE = 0x02;

    /** A-Glonass positioning protocol bitmask for LTE Positioning Protocol User Plane. */
    const int GLONASS_POS_PROTOCOL_LPP_UPLANE = 0x04;

    /**
     * This method sets the SUPL version requested by Carrier. The GNSS HAL must use this version
     * of the SUPL protocol if supported.
     *
     * @param version SUPL version requested by carrier. This is a bit mask with bits 0:7
     * representing a service indicator field, bits 8:15 representing the minor version and bits
     * 16:23 representing the major version.
     */
    void setSuplVersion(in int version);

    /**
     * This method sets the SUPL mode.
     *
     * @param mode Bitmask that specifies the SUPL mode which is set with the SUPL_MODE_* constants.
     */
    void setSuplMode(in int mode);

    /**
     * This method sets the LTE Positioning Profile configuration.
     *
     * @param lppProfile Bitmask that specifies the LTE Positioning Profile configuration to be set
     * as per the LPP_PROFILE_* constants. If none of the bits are set, the default setting is
     * Radio Resource Location Protocol (RRLP).
     */
    void setLppProfile(in int lppProfile);

    /**
     * This method selects positioning protocol on A-Glonass system.
     *
     * @param protocol Bitmask that specifies the positioning protocol to be set as per
     * GLONASS_POS_PROTOCOL_* constants.
     */
    void setGlonassPositioningProtocol(in int protocol);

    /**
     * This method configures which PDN to use.
     *
     * @param enable Use emergency PDN if true and regular PDN if false.
     */
    void setEmergencySuplPdn(in boolean enable);

    /**
     * This method sets the emergency session extension duration. The GNSS HAL
     * implementation must serve emergency SUPL and Control Plane network initiated
     * location requests for this extra duration after the user initiated emergency
     * session ends.
     *
     * @param emergencyExtensionSeconds Number of seconds to extend the emergency
     * session duration post emergency call.
     */
    void setEsExtensionSec(in int emergencyExtensionSeconds);

    /**
     * Injects a vector of BlocklistedSource(s) which the HAL must not use to calculate the
     * GNSS location output.
     *
     * The superset of all satellite sources provided, including wildcards, in the latest call
     * to this method, is the set of satellites sources that must not be used in calculating
     * location.
     *
     * All measurements from the specified satellites, across frequency bands, are blocklisted
     * together.
     *
     * If this method is never called after the IGnssConfiguration.hal connection is made on boot,
     * or is called with an empty vector, then no satellites are to be blocklisted as a result of
     * this API.
     *
     * This blocklist must be considered as an additional source of which satellites
     * should not be trusted for location on top of existing sources of similar information
     * such as satellite broadcast health being unhealthy and measurement outlier removal.
     *
     * @param blocklist The BlocklistedSource(s) of satellites the HAL must not use.
     */
    void setBlocklist(in BlocklistedSource[] blocklist);
}