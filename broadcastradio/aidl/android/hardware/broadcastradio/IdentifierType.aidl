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
 * Type of program identifier component.
 *
 * Each identifier type corresponds to exactly one radio technology,
 * i.e. DAB_ENSEMBLE is specifically for DAB.
 *
 * VENDOR identifier types must be opaque to the framework.
 *
 * The value format for each (but VENDOR_*) identifier is strictly defined
 * to maintain interoperability between devices made by different vendors.
 *
 * All other values are reserved for future use.
 * Values not matching any enumerated constant must be ignored.
 */
@VintfStability
@Backing(type="int")
@JavaDerive(equals=true, toString=true)
enum IdentifierType {
    /**
     * Primary/secondary identifier for vendor-specific radio technology.
     * The value format is determined by a vendor.
     *
     * The vendor identifiers have limited serialization capabilities - see
     * ProgramSelector description.
     */
    VENDOR_START = 1000,

    /**
     * See VENDOR_START
     */
    VENDOR_END = 1999,

    /**
     * Undefined identifier type.
     */
    INVALID = 0,

    /**
     * Primary identifier for analogue (without RDS) AM/FM stations:
     * frequency in kHz.
     *
     * This identifier also contains band information:
     *  - <500kHz: AM LW;
     *  - 500kHz - 1705kHz: AM MW;
     *  - 1.71MHz - 30MHz: AM SW;
     *  - >60MHz: FM.
     */
    AMFM_FREQUENCY_KHZ,

    /**
     * 16bit primary identifier for FM RDS station.
     */
    RDS_PI,

    /**
     * 64bit compound primary identifier for HD Radio.
     *
     * Consists of (from the LSB):
     * - 32bit: Station ID number;
     * - 4bit: HD Radio subchannel;
     * - 18bit: AMFM_FREQUENCY_KHZ.
     *
     * While station ID number should be unique globally, it sometimes get
     * abused by broadcasters (i.e. not being set at all). To ensure local
     * uniqueness, AMFM_FREQUENCY_KHZ was added here. Global uniqueness is
     * a best-effort - see HD_STATION_NAME.
     *
     * HD Radio subchannel is a value in range 0-7.
     * This index is 0-based (where 0 is MPS and 1..7 are SPS),
     * as opposed to HD Radio standard (where it's 1-based).
     *
     * The remaining bits should be set to zeros when writing on the chip side
     * and ignored when read.
     */

    HD_STATION_ID_EXT,

    /**
     * 64bit additional identifier for HD Radio.
     *
     * Due to Station ID abuse, some HD_STATION_ID_EXT identifiers may be not
     * globally unique. To provide a best-effort solution, a short version of
     * station name may be carried as additional identifier and may be used
     * by the tuner hardware to double-check tuning.
     *
     * The name is limited to the first 8 A-Z0-9 characters (lowercase letters
     * must be converted to uppercase). Encoded in little-endian ASCII:
     * the first character of the name is the LSB.
     *
     * For example: "Abc" is encoded as 0x434241.
     */
    HD_STATION_NAME,

    /**
     * 44bit compound primary identifier for Digital Audio Broadcasting and
     * Digital Multimeida Broadcasting.
     *
     * Consists of (from the LSB):
     * - 32bit: SId;
     * - 8bit: ECC code;
     * - 4bit: SCIdS.
     *
     * SCIdS (Service Component Identifier within the Service) value
     * of 0 represents the main service, while 1 and above represents
     * secondary services.
     *
     * The remaining bits should be set to zeros when writing on the chip side
     * and ignored when read.
     */
    DAB_SID_EXT,

    /**
     * 16bit
     */
    DAB_ENSEMBLE,

    /**
     * 12bit
     */
    DAB_SCID,

    /**
     * kHz (see AMFM_FREQUENCY_KHZ)
     */
    DAB_FREQUENCY_KHZ,

    /**
     * 24bit primary identifier for Digital Radio Mondiale.
     */
    DRMO_SERVICE_ID,

    /**
     * kHz (see AMFM_FREQUENCY_KHZ)
     */
    DRMO_FREQUENCY_KHZ,

    /**
     * 32bit primary identifier for SiriusXM Satellite Radio.
     *
     * @deprecated SiriusXM Satellite Radio is not supported.
     */
    SXM_SERVICE_ID = DRMO_FREQUENCY_KHZ + 2,

    /**
     * 0-999 range
     *
     * @deprecated SiriusXM Satellite Radio is not supported.
     */
    SXM_CHANNEL,

    /**
     * 64bit additional identifier for HD Radio representing station location.
     *
     * Consists of (from the LSB):
     * - 4 bit: Bits 0:3 of altitude
     * - 13 bit: Fractional bits of longitude
     * - 8 bit: Integer bits of longitude
     * - 1 bit: 0 for east and 1 for west for longitude
     * - 1 bit: 0, representing latitude
     * - 5 bit: pad of zeros separating longitude and latitude
     * - 4 bit: Bits 4:7 of altitude
     * - 13 bit: Fractional bits of latitude
     * - 8 bit: Integer bits of latitude
     * - 1 bit: 0 for north and 1 for south for latitude
     * - 1 bit: 1, representing latitude
     * - 5 bit: pad of zeros
     *
     * This format is defined in NRSC-5-C document: SY_IDD_1020s.
     *
     * Due to Station ID abuse, some HD_STATION_ID_EXT identifiers may be not
     * globally unique. To provide a best-effort solution, the station’s
     * broadcast antenna containing the latitude and longitude may be carried
     * as additional identifier and may be used by the tuner hardware to
     * double-check tuning.
     */
    HD_STATION_LOCATION,
}
