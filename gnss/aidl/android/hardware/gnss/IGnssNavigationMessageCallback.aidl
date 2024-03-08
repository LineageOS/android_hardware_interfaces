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

package android.hardware.gnss;

/**
 * Represents a GNSS navigation message (or a fragment of it).
 *
 * @hide
 */
@VintfStability
interface IGnssNavigationMessageCallback {
    /**
     * Represents a reading of GNSS measurements. For devices launched in Android Q or newer, it is
     * mandatory that these be provided, on request, when the GNSS receiver is searching/tracking
     * signals.
     *
     * - Reporting of GNSS constellation measurements is mandatory.
     * - Reporting of all tracked constellations are encouraged.
     */
    @VintfStability
    parcelable GnssNavigationMessage {
        /**
         * Status of Navigation Message
         *
         * When a message is received properly without any parity error in its
         * navigation words, the status must be set to PARITY_PASSED.
         *
         * If a message is received with words that failed a parity check, but the GNSS
         * receiver has corrected those words, the status must be set to PARITY_REBUILT.
         *
         * Do not send any navigation message that contains words with parity errors
         * that cannot be corrected.
         */
        const int STATUS_PARITY_PASSED = (1 << 0);
        const int STATUS_PARITY_REBUILT = (1 << 1);
        const int STATUS_UNKNOWN = 0;

        /**
         * Enumeration of available values to indicate the GNSS Navigation message
         * types.
         *
         * For convenience, first byte is the GnssConstellationType on which that signal
         * is typically transmitted.
         */
        @VintfStability
        @Backing(type="int")
        enum GnssNavigationMessageType {
            UNKNOWN = 0,

            /** GPS L1 C/A message contained in the structure.  */
            GPS_L1CA = 0x0101,

            /** GPS L2-CNAV message contained in the structure. */
            GPS_L2CNAV = 0x0102,

            /** GPS L5-CNAV message contained in the structure. */
            GPS_L5CNAV = 0x0103,

            /* SBAS message contained in the structure. */
            SBS = 0x0201,

            /** GPS CNAV-2 message contained in the structure. */
            GPS_CNAV2 = 0x0104,

            /** Glonass L1 CA message contained in the structure. */
            GLO_L1CA = 0x0301,

            /** QZSS L1 C/A message contained in the structure. */
            QZS_L1CA = 0x0401,

            /** Beidou D1 message contained in the structure. */
            BDS_D1 = 0x0501,

            /** Beidou D2 message contained in the structure. */
            BDS_D2 = 0x0502,

            /** Beidou CNAV1 message contained in the structure. */
            BDS_CNAV1 = 0x0503,

            /** Beidou CNAV2 message contained in the structure. */
            BDS_CNAV2 = 0x0504,

            /** Galileo I/NAV message contained in the structure. */
            GAL_I = 0x0601,

            /** Galileo F/NAV message contained in the structure. */
            GAL_F = 0x0602,

            /**
             * NavIC L5 C/A message contained in the structure.
             * @deprecated Use IRN_L5 instead.
             */
            IRN_L5CA = 0x0701,

            /** NavIC L5 message contained in the structure. */
            IRN_L5 = 0x0702,

            /** NavIC L1 message contained in the structure. */
            IRN_L1 = 0x0703,
        }

        /**
         * Satellite vehicle ID number, as defined in GnssSvInfo::svid
         *
         * This is a mandatory value.
         */
        int svid;

        /**
         * The type of message contained in the structure.
         *
         * This is a mandatory value.
         */
        GnssNavigationMessageType type;

        /**
         * The status of the received navigation message.
         *
         * It is a bitfield of constants with prefix "STATUS_" defined in this structure.
         *
         * No need to send any navigation message that contains words with parity
         * errors that cannot be corrected.
         */
        int status;

        /**
         * Message identifier. It provides an index so the complete Navigation
         * Message can be assembled.
         *
         * - For GNSS L1 C/A subframe 4 and 5, this value corresponds to the 'frame
         *   id' of the navigation message, in the range of 1-25 (Subframe 1, 2, 3
         *   does not contain a 'frame id' and this value can be set to -1.)
         *
         * - For Glonass L1 C/A, this refers to the frame ID, in the range of 1-5.
         *
         * - For BeiDou D1, this refers to the frame number in the range of 1-24
         *
         * - For Beidou D2, this refers to the frame number, in the range of 1-120
         *
         * - For Galileo F/NAV nominal frame structure, this refers to the subframe
         *   number, in the range of 1-12
         *
         * - For Galileo I/NAV nominal frame structure, this refers to the subframe
         *   number in the range of 1-24
         *
         * - For SBAS and Beidou CNAV2, this is unused and can be set to -1.
         *
         * - For QZSS L1 C/A subframe 4 and 5, this value corresponds to the 'frame id' of the
         *   navigation message, in the range of 1-25. (Subframe 1, 2, 3 does not contain a 'frame
         *   id' and this value can be set to -1.)
         *
         * - For Beidou CNAV1 this refers to the page type number in the range of 1-63.
         *
         * - For NavIC L5 subframe 3 and 4, this value corresponds to the Message Id of the
         *   navigation message, in the range of 1-63. (Subframe 1 and 2 does not contain a message
         *   type id and this value can be set to -1.)
         * - For NavIC L1 subframe 3, this value corresponds to the Message Id of the navigation
         *   message, in the range of 1-63. (Subframe 1 and 2 does not contain a message type id and
         *   this value can be set to -1.)
         */
        int messageId;

        /**
         * Sub-message identifier. If required by the message 'type', this value contains a
         * sub-index within the current message (or frame) that is being transmitted.
         *
         * - For GNSS L1 C/A, BeiDou D1 & BeiDou D2, the submessage id corresponds to the subframe
         *   number of the navigation message, in the range of 1-5.
         *
         * - For Glonass L1 C/A, this refers to the String number, in the range from 1-15.
         *
         * - For Galileo F/NAV, this refers to the page type in the range 1-6.
         *
         * - For Galileo I/NAV, this refers to the word type in the range 0-10+. A value of 0 is
         *   only allowed if the Satellite is transmiting a Spare Word.
         *
         * - For Galileo in particular, the type information embedded within the data bits may be
         *   even more useful in interpretation, than the nominal page and word types provided in
         *   this field.
         *
         * - For SBAS, the submessage id corresponds to the message type, in the range 1-63.
         *
         * - For Beidou CNAV1, the submessage id corresponds to the subframe number of the
         *   navigation message, in the range of 1-3.
         *
         * - For Beidou CNAV2, the submessage id corresponds to the message type, in the range 1-63.
         *
         * - For NavIC L5, the submessage id corresponds to the subframe number of the navigation
         *   message, in the range of 1-4.
         * - For NavIC L1, the submessage id corresponds to the subframe number of the navigation
         *   message, in the range of 1-3.
         */
        int submessageId;

        /**
         * The data of the reported GNSS message. The bytes (or words) are specified
         * using big endian format (MSB first).
         *
         * - For GNSS L1 C/A, NavIC L5, Beidou D1 & Beidou D2, each subframe contains 10 30-bit
         *   words. Each word (30 bits) must fit into the last 30 bits in a
         *   4-byte word (skip B31 and B32), with MSB first, for a total of 40
         *   bytes, covering a time period of 6, 6, and 0.6 seconds, respectively.
         *   The standard followed is 1995 SPS Signal specification.
         *
         * - For Glonass L1 C/A, each string contains 85 data bits, including the
         *   checksum.  These bits must fit into 11 bytes, with MSB first (skip
         *   B86-B88), covering a time period of 2 seconds.
         *   The standard followed is Glonass Interface Control Document Edition 5.1.
         *
         * - For Galileo F/NAV, each word consists of 238-bit (sync & tail symbols
         *   excluded). Each word must fit into 30-bytes, with MSB first (skip
         *   B239, B240), covering a time period of 10 seconds. The standard
         *   followed is European GNSS(Galileo) Signal in Space Interface
         *   Control Document Issue 1.2.
         *
         * - For Galileo I/NAV, each page contains 2 page parts, even and odd, with
         *   a total of 2x114 = 228 bits, (sync & tail excluded) that must fit
         *   into 29 bytes, with MSB first (skip B229-B232). The standard followed
         *   is same as above.
         *
         * - For SBAS, each block consists of 250 data bits, that should be fit into 32 bytes. MSB
         *   first (skip B251-B256).
         *
         * - For Beidou CNAV1, subframe #1 consists of 14 data bits, that should be fit into 2
         *   bytes. MSB first (skip B15-B16).  subframe #2 consists of 600 bits that should be fit
         *   into 75 bytes. subframe #3 consists of 264 data bits that should be fit into 33 bytes.
         *
         * - For Beidou CNAV2, each subframe consists of 288 data bits, that should be fit into 36
         *   bytes.
         *
         * - For NavIC L1, subframe #1 consists of 9 data bits that should be fit into 2 bytes (skip
         *   B10-B16). subframe #2 consists of 600 bits that should be fit into 75 bytes. subframe
         *   #3 consists of 274 data bits that should be fit into 35 bytes (skip B275-B280).
         *
         * The data reported here must be the raw data as demodulated by the GNSS receiver, not data
         * received from an external source (i.e. not from a server download.)
         */
        byte[] data;
    }

    /**
     * The callback to report an available fragment of a GNSS navigation messages
     * from the HAL.
     *
     * @param message - The GNSS navigation submessage/subframe representation.
     */
    void gnssNavigationMessageCb(in GnssNavigationMessage message);
}
