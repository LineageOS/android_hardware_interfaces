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

import android.hardware.gnss.GnssConstellationType;

/**
 * Represents a GNSS signal type.
 */
@VintfStability
parcelable GnssSignalType {
    /**
     * Constellation type of the SV that transmits the signal.
     */
    GnssConstellationType constellation = GnssConstellationType.UNKNOWN;

    /**
     * Carrier frequency of the signal tracked, for example it can be the
     * GPS central frequency for L1 = 1575.45 MHz, or L2 = 1227.60 MHz, L5 =
     * 1176.45 MHz, varying GLO channels, etc. If the field is not set, it
     * is the primary common use central frequency, e.g. L1 = 1575.45 MHz
     * for GPS.
     *
     * For an L1, L5 receiver tracking a satellite on L1 and L5 at the same
     * time, two raw measurement structs must be reported for this same
     * satellite, in one of the measurement structs, all the values related
     * to L1 must be filled, and in the other all of the values related to
     * L5 must be filled.
     */
    double carrierFrequencyHz;

    /**
     * GNSS signal code type "A" representing GALILEO E1A, GALILEO E6A, IRNSS L5A, IRNSS SA.
     */
    const @utf8InCpp String CODE_TYPE_A = "A";

    /**
     * GNSS signal code type "B" representing GALILEO E1B, GALILEO E6B, IRNSS L5B, IRNSS SB.
     */
    const @utf8InCpp String CODE_TYPE_B = "B";

    /**
     * GNSS signal code type "C" representing GPS L1 C/A,  GPS L2 C/A, GLONASS G1 C/A,
     * GLONASS G2 C/A, GALILEO E1C, GALILEO E6C, SBAS L1 C/A, QZSS L1 C/A, IRNSS L5C.
     */
    const @utf8InCpp String CODE_TYPE_C = "C";

    /**
     * GNSS signal code type "D" representing BDS B1C D.
     */
    const @utf8InCpp String CODE_TYPE_D = "D";

    /**
     * GNSS signal code type "I" representing GPS L5 I, GLONASS G3 I, GALILEO E5a I, GALILEO E5b I,
     * GALILEO E5a+b I, SBAS L5 I, QZSS L5 I, BDS B1 I, BDS B2 I, BDS B3 I.
     */
    const @utf8InCpp String CODE_TYPE_I = "I";

    /**
     * GNSS signal code type "L" representing GPS L1C (P), GPS L2C (L), QZSS L1C (P), QZSS L2C (L),
     * LEX(6) L.
     */
    const @utf8InCpp String CODE_TYPE_L = "L";

    /**
     * GNSS signal code type "M" representing GPS L1M, GPS L2M.
     */
    const @utf8InCpp String CODE_TYPE_M = "M";

    /**
     * GNSS signal code type "N" representing GPS L1 codeless, GPS L2 codeless.
     */
    const @utf8InCpp String CODE_TYPE_N = "N";

    /**
     * GNSS signal code type "P" representing GPS L1P, GPS L2P, GLONASS G1P, GLONASS G2P, BDS B1C P.
     */
    const @utf8InCpp String CODE_TYPE_P = "P";

    /**
     * GNSS signal code type "Q" representing GPS L5 Q, GLONASS G3 Q, GALILEO E5a Q, GALILEO E5b Q,
     * GALILEO E5a+b Q, SBAS L5 Q, QZSS L5 Q, BDS B1 Q, BDS B2 Q, BDS B3 Q.
     */
    const @utf8InCpp String CODE_TYPE_Q = "Q";

    /**
     * GNSS signal code type "S" represents GPS L1C (D), GPS L2C (M), QZSS L1C (D), QZSS L2C (M),
     * LEX(6) S.
     */
    const @utf8InCpp String CODE_TYPE_S = "S";

    /**
     * GNSS signal code type "W" representing GPS L1 Z-tracking, GPS L2 Z-tracking.
     */
    const @utf8InCpp String CODE_TYPE_W = "W";

    /**
     * GNSS signal code type "X" representing GPS L1C (D+P), GPS L2C (M+L), GPS L5 (I+Q),
     * GLONASS G3 (I+Q), GALILEO E1 (B+C), GALILEO E5a (I+Q), GALILEO E5b (I+Q), GALILEO E5a+b(I+Q),
     * GALILEO E6 (B+C), SBAS L5 (I+Q), QZSS L1C (D+P), QZSS L2C (M+L), QZSS L5 (I+Q),
     * LEX(6) (S+L), BDS B1 (I+Q), BDS B1C (D+P), BDS B2 (I+Q), BDS B3 (I+Q), IRNSS L5 (B+C).
     */
    const @utf8InCpp String CODE_TYPE_X = "X";

    /**
     * GNSS signal code type "Y" representing GPS L1Y, GPS L2Y.
     */
    const @utf8InCpp String CODE_TYPE_Y = "Y";

    /**
     * GNSS signal code type "Z" representing GALILEO E1 (A+B+C), GALILEO E6 (A+B+C), QZSS L1-SAIF.
     */
    const @utf8InCpp String CODE_TYPE_Z = "Z";

    /**
     * GNSS signal code type "UNKNOWN" representing the GNSS Measurement's code type is unknown.
     */
    const @utf8InCpp String CODE_TYPE_UNKNOWN = "UNKNOWN";

    /**
     * The type of code that is currently being tracked in the GNSS signal.
     *
     * For high precision applications the type of code being tracked needs to be considered
     * in-order to properly apply code specific corrections to the pseudorange measurements.
     *
     * The value is one of the constant Strings with prefix CODE_TYPE_ defined in this parcelable.
     *
     * This is used to specify the observation descriptor defined in GNSS Observation Data File
     * Header Section Description in the RINEX standard (Version 3.XX). In RINEX Version 3.03,
     * in Appendix Table A2 Attributes are listed as uppercase letters (for instance, "A" for
     * "A channel"). In the future, if for instance a code "G" was added in the official RINEX
     * standard, "G" could be specified here.
     */
    @utf8InCpp String codeType;
}
