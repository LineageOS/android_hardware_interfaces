/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.uwb.fira_android;

/**
 * Android specific capability TLV types in UCI command:
 * GID: 0000b (UWB Core Group)
 * OID: 000011b (CORE_GET_CAPS_INFO_CMD)
 *
 * For FIRA params, please refer to params mentioned in CR 287.
 *
 * Values expected for each type are mentioned in the docs below and the constants
 * used are defined in UwbVendorCapabilityTlvValues enum.
 */
@VintfStability
@Backing(type="int")
enum UwbVendorCapabilityTlvTypes {

    /*********************************************
     * Protocol agnostic
     ********************************************/
    /**
     * 1 byte value to indicate support for power stats query
     * Values:
     *  1 - Feature supported.
     *  0 - Feature not supported.
     */
    SUPPORTED_POWER_STATS_QUERY = 0xC0,

    /*********************************************
     * CCC specific
     ********************************************/

    /**
     * 1 byte bitmask with a list of supported chaps per slot
     * Bitmap of supported values of Slot durations as a multiple of TChap,
     * NChap_per_Slot as defined in CCC Specification.
     * Each “1” in this bit map corresponds to a specific
     * value of NChap_per_Slot where:
     * 0x01 = “3”,
     * 0x02 = “4”,
     * 0x04= “6”,
     * 0x08 =“8”,
     * 0x10 =“9”,
     * 0x20 = “12”,
     * 0x40 = “24”,
     * 0x80 is reserved.
     */
    CCC_SUPPORTED_CHAPS_PER_SLOT = 0xA0,

    /**
     * 4 byte bitmask with a list of supported sync codes
     * Bitmap of SYNC code indices that can be used.
     * The position of each “1” in this bit pattern
     * corresponds to the index of a SYNC code that
     * can be used, where:
     * 0x00000001 = “1”,
     * 0x00000002 = “2”,
     * 0x00000004 = “3”,
     * 0x00000008 = “4”,
     * ….
     * 0x40000000 = “31”,
     * 0x80000000 = “32”
     * Refer to IEEE 802.15.4-2015 and CCC
     * Specification for SYNC code index definition
     */
    CCC_SUPPORTED_SYNC_CODES = 0xA1,

    /**
     * 1 byte bitmask with a list of supported hopping config modes and sequences.
     * [b7 b6 b5] : bitmask of hopping modes the
     * device offers to use in the ranging session
     * 100 - No Hopping
     * 010 - Continuous Hopping
     * 001 - Adaptive Hopping
     * [b4 b3 b2 b1 b0] : bit mask of hopping
     * sequences the device offers to use in the
     * ranging session
     * b4=1 is always set because of the default
     * hopping sequence. Support for it is mandatory.
     * b3=1 is set when the optional AES based
     * hopping sequence is supported.
     */
    CCC_SUPPORTED_HOPPING_CONFIG_MODES_AND_SEQUENCES = 0xA2,

    /**
     * 1 byte bitmask with list of supported channels
     * Bitmap of supported UWB channels. Each “1” in
     * this bit map corresponds to a specific value of
     * UWB channel where:
     * 0x01 = "Channel 5"
     * 0x02 = "Channel 9"
     */
    CCC_SUPPORTED_CHANNELS = 0xA3,

    /**
     * 2 byte tuple {major_version (1 byte), minor_version (1 byte)} array with list of supported
     * CCC versions
     */
    CCC_SUPPORTED_VERSIONS = 0xA4,

    /**
     * byte array with a list of supported UWB configs
     *
     * UWB configurations are define in chapter
     * "21.4 UWB Frame Elements" of the CCC
     * specification. Configuration 0x0000 is
     * mandatory for device and vehicle, configuration
     * 0x0001 is mandatory for the device, optional for
     * the vehicle.
     */
    CCC_SUPPORTED_UWB_CONFIGS = 0xA5,

    /**
     * 1 byte tuple {initiator_tx (4 bits), responder_tx (4 bits)} array with list of supported
     * pulse shape combos
     * Values:
     *  PULSE_SHAPE_SYMMETRICAL_ROOT_RAISED_COSINE = 0
     *  PULSE_SHAPE_PRECURSOR_FREE = 1
     *  PULSE_SHAPE_PRECURSOR_FREE_SPECIAL = 2
     */
    /**  */
    CCC_SUPPORTED_PULSE_SHAPE_COMBOS = 0xA6,

    /** Int value for indicating supported ran multiplier */
    CCC_SUPPORTED_RAN_MULTIPLIER = 0xA7,

    /**
     * Int value to indicate supported max number of ccc ranging sessions
     */
    CCC_SUPPORTED_MAX_RANGING_SESSION_NUMBER = 0xA8,

    /**
     * Int value to indicate the smallest supported CCC uwb initiation time in ms
     */
    CCC_SUPPORTED_MIN_UWB_INITIATION_TIME_MS = 0xA9,

    /**
     * Byte array of channels supported by the device, ordered by priority from highest to lowest.
     */
    CCC_PRIORITIZED_CHANNEL_LIST = 0xAA,

    /*********************************************
     * RADAR specific
     ********************************************/
    /**
     * 1 byte bitmask to indicate the supported Radar data types.
     * Each "1" in this bitmap corresponds to a specific radar data type where:
     * 0x01 = "Radar Sweep Samples",
     */
    RADAR_SUPPORT = 0xB0,

    /*********************************************
     * FIRA specific
     ********************************************/
    /**
     * 1 byte value to indicate support for antenna interleaving
     * feature.
     * Values:
     *  1 - Feature supported.
     *  0 - Feature not supported.
     */
    SUPPORTED_AOA_RESULT_REQ_ANTENNA_INTERLEAVING = 0xE3,

    /**
     * 4 byte value to indicate supported min ranging interval in ms.
     */
    SUPPORTED_MIN_RANGING_INTERVAL_MS = 0xE4,

    /**
     * 4 byte bitmask to indicate the supported RANGE_DATA_NTF_CONFIG values
     *
     * Bitmask where each bit corresponds to values used in
     * RANGE_DATA_NTF_CONFIG in SET_APP_CFG_CMD
     */
    SUPPORTED_RANGE_DATA_NTF_CONFIG = 0xE5,

    /**
     * 1 byte bitmask to indicate the supported RSSI_REPORTING values
     * Values:
     * 1 - Feature supported.
     * 0 - Feature not supported.
     */
    SUPPORTED_RSSI_REPORTING = 0xE6,

    /**
     * 1 byte value to indicate support for diagnostics feature.
     * Values:
     *  1 - Feature supported.
     *  0 - Feature not supported.
     */
    SUPPORTED_DIAGNOSTICS = 0xE7,

    /**
     * 4 byte value to indicate supported min slot duration in rstu.
     */
    SUPPORTED_MIN_SLOT_DURATION_RSTU = 0xE8,

    /**
     * Int value to indicate supported max number of fira ranging sessions
     */
    SUPPORTED_MAX_RANGING_SESSION_NUMBER = 0xE9,

    /**
     * 2 byte bitmask to indicate the channels that support AoA.
     * Each "1" in this bitmap corresponds to a specific UWB channel where:
     *  0x01 = "Channel 5",
     *  0x02 = "Channel 6",
     *  0x04 = "Channel 8",
     *  0x08 = "Channel 9",
     *  0x10 = "Channel 10",
     *  0x20 = "Channel 12",
     *  0x40 = "Channel 13",
     *  0x80 = "Channel 14"
     */
    SUPPORTED_CHANNELS_AOA = 0xEA,
}
