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

/**
 * An element of metadata array.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
union Metadata {
    /**
     * RDS PS (string)
     */
    String rdsPs;

    /**
     * RDS PTY (uint8_t)
     */
    int rdsPty;

    /**
     * RBDS PTY (uint8_t)
     */
    int rbdsPty;

    /**
     * RDS RT (string)
     */
    String rdsRt;

    /**
     * Song title (string)
     */
    String songTitle;

    /**
     * Artist name (string)
     */
    String songArtist;

    /**
     * Album name (string)
     */
    String songAlbum;

    /**
     * Station icon (uint32_t, see {@link IBroadcastRadio#getImage})
     */
    int stationIcon;

    /**
     * Album art (uint32_t, see {@link IBroadcastRadio#getImage})
     */
    int albumArt;

    /**
     * Station name.
     *
     * <p>This is a generic field to cover any radio technology.
     *
     * <p>Note: If the program name has the same content as dab*Name or ({@link Metadata#rdsPs},
     * it may not be present, to preserve space - framework must repopulate
     * it on the client side.
     */
    String programName;

    /**
     * DAB ensemble name (string)
     */
    String dabEnsembleName;

    /**
     * DAB ensemble name abbreviated (string).
     *
     * <p>Note: The string must be up to 8 characters long.
     *
     * <p>Note: If the short variant is present, the long ({@link Metadata#dabEnsembleName})
     * one must be present as well.
     */
    String dabEnsembleNameShort;

    /**
     * DAB service name (string)
     */
    String dabServiceName;

    /**
     * DAB service name abbreviated (string)
     *
     * <p>Note: The string must be up to 8 characters long.
     */
    String dabServiceNameShort;

    /**
     * DAB component name (string)
     */
    String dabComponentName;

    /**
     * DAB component name abbreviated (string)
     *
     * <p>Note: The string must be up to 8 characters long.
     */
    String dabComponentNameShort;

    /**
     * Genre of the current audio piece (string)
     *
     * <p>(see NRSC-G200-A and id3v2.3.0 for more info)
     */
    String genre;

    /**
     * Short context description of comment (string)
     *
     * <p>Comment could relate to the current audio program content, or it might
     * be unrelated information that the station chooses to send. It is
     * composed of short content description and actual text (see NRSC-G200-A
     * and id3v2.3.0 for more info).
     */
    String commentShortDescription;

    /**
     * Actual text of comment (string)
     *
     * @see #commentShortDescription
     */
    String commentActualText;

    /**
     * Commercial (string)
     *
     * <p>Commercial is application specific and generally used to facilitate the
     * sale of products and services (see NRSC-G200-A and id3v2.3.0 for more info).
     */
    String commercial;

    /**
     * HD Unique File Identifiers (Array of strings)
     *
     * <p>Unique File Identifier (UFID) can be used to transmit an alphanumeric
     * identifier of the current content, or of an advertised product or service
     * (see NRSC-G200-A and id3v2.3.0 for more info).
     */
    String[] ufids;

    /**
     * HD short station name or HD universal short station name
     *
     * <p>It can be up to 12 characters (see SY_IDD_1020s for more info).
     */
    String hdStationNameShort;

    /**
     * HD long station name, HD station slogan or HD station message
     *
     * <p>(see SY_IDD_1020s for more info)
     */
    String hdStationNameLong;

    /**
     * Bit mask of all HD Radio subchannels available (uint8_t)
     *
     * <p>Bit {@link HdSubChannel#HD1} from LSB represents the availability
     * of HD-1 subchannel (main program service, MPS). Bits
     * {@link HdSubChannel#HD2} to {@link HdSubChannel#HD8} from LSB represent
     * HD-2 to HD-8 subchannel (supplemental program services, SPS)
     * respectively.
     */
    int hdSubChannelsAvailable;
}
