/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.gnss.gnss_assistance;

/**
 * Contains the real time integrity status of a GNSS satellite based on
 * notice advisory.
 *
 * @hide
 */
@VintfStability
parcelable RealTimeIntegrityModel {
    /**
     * Pseudo-random or satellite ID number for the satellite, a.k.a. Space Vehicle (SV), or
     * OSN number for Glonass. The distinction is made by looking at the constellation field.
     * Values must be in the range of:
     *
     * - GNSS:    1-32
     * - GLONASS: 1-25
     * - QZSS:    183-206
     * - Galileo: 1-36
     * - Beidou:  1-63
     */
    int svid;

    /** Indicates whether the satellite is currently usable for navigation. */
    boolean usable;

    /** UTC timestamp (in seconds) when the advisory was published. */
    long publishDateSeconds;

    /** UTC timestamp (in seconds) for the start of the event. */
    long startDateSeconds;

    /** UTC timestamp (in seconds) for the end of the event. */
    long endDateSeconds;

    /**
     * Abbreviated type of the advisory, providing a concise summary of the event.
     * This field follows different definitions depending on the GNSS constellation:
     *  - GPS: See NANU type definitions
     *    (https://www.navcen.uscg.gov/nanu-abbreviations-and-descriptions)
     *  - Galileo: See NAGU type definitions
     *    (https://www.gsc-europa.eu/system-service-status/nagu-information)
     *  - QZSS: See NAQU type definitions (https://sys.qzss.go.jp/dod/en/naqu/type.html)
     *  - BeiDou: Not used; set to an empty string.
     */
    String advisoryType;

    /**
     *  Unique identifier for the advisory within its constellation's system.
     *  For BeiDou, this is not used and should be an empty string.
     */
    String advisoryNumber;
}
