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
 * Android specific capability params expected in UCI command:
 * GID: 0000b (UWB Core Group)
 * OID: 000011b (CORE_GET_CAPS_INFO_CMD)
 * Values expected for each type should correspond to values used in the FIRA UCI specification.
 *
 * For ex, If the device supports responder and initiator device roles. Use this TLV to
 * indicate that: * {SUPPORTED_DEVICE_ROLES, 2, [0x0 // responder, 0x1 // initiator]}}
 */
@VintfStability
@Backing(type="int")
enum UwbVendorCapabilityTlvTypes {
    /** byte array with list of supported channels */
    SUPPORTED_CHANNELS = 0x0,
    /**
     * int bitmask of supported AOA modes
     * Values:
     *  HAS_AZIMUTH_SUPPORT = 1,
     *  HAS_ELEVATION_SUPPORT = (1 << 1)
     *  HAS_FOM_SUPPORT = (1 << 2)
     *  HAS_FULL_AZIMUTH_SUPPORT = (1 << 3)
     */
    SUPPORTED_AOA_MODES = 0x1,
    /**
     * int bitmask of supported device roles
     * Values:
     *   HAS_CONTROLEE_INITIATOR_SUPPORT = 1
     *   HAS_CONTROLEE_RESPONDER_SUPPORT = (1 << 1)
     *   HAS_CONTROLLER_INITIATOR_SUPPORT = (1 << 2)
     *   HAS_CONTROLLER_RESPONDER_SUPPORT= (1 << 3)
     */
    SUPPORTED_DEVICE_ROLES = 0x2,
    /** byte value of 1 to indicate support for block striding */
    SUPPORTS_BLOCK_STRIDING = 0x3,
    /** byte value of 1 to indicate support for non deferred mode */
    SUPPORTS_NON_DEFERRED_MODE = 0x4,
    /** byte value of 1 to indicate support for adaptive payload power */
    SUPPORTS_ADAPTIVE_PAYLOAD_POWER = 0x5,
    /** Int value for indicating initiation time */
    INITIATION_TIME_MS = 0x6,
    /**
     * int bitmask of supported mac fcs crc types
     * Values:
     *  HAS_CRC_16_SUPPORT = 1
     *  HAS_CRC_32_SUPPORT = (1 << 1)
     */
    SUPPORTED_MAC_FCS_CRC_TYPES = 0x7,
    /**
     * int bitmask of supported multi node modes
     * Values:
     *  HAS_UNICAST_SUPPORT = 1
     *  HAS_ONE_TO_MANY_SUPPORT = (1 << 1)
     *  HAS_MANY_TO_MANY_SUPPORT = (1 << 2)
     */
    SUPPORTED_MULTI_NODE_MODES = 0x8,
    /**
     * int bitmask of supported preamble modes
     * Values:
     *  HAS_32_SYMBOLS_SUPPORT = 1
     *  HAS_64_SYMBOLS_SUPPORT = (1 << 1)
     */
    SUPPORTED_PREAMBLE_MODES = 0x9,
    /**
     * int bitmask of supported prf modes
     * Values:
     *  HAS_BPRF_SUPPORT = 1
     *  HAS_HPRF_SUPPORT = (1 << 1)
     */
    SUPPORTED_PRF_MODES = 0xA,
    /**
     * int bitmask of supported ranging round usage modes
     * Values:
     *  HAS_DS_TWR_SUPPORT = 1
     *  HAS_SS_TWR_SUPPORT = (1 << 1)
     */
    SUPPORTED_RANGING_ROUND_USAGE_MODES = 0xB,
    /**
     * int bitmask of supported rframe modes
     * Values:
     *  HAS_SP0_RFRAME_SUPPORT = 1
     *  HAS_SP1_RFRAME_SUPPORT = (1 << 1),
     *  HAS_SP3_RFRAME_SUPPORT = (1 << 3)
     */
    SUPPORTED_RFRAME_MODES = 0xC,
    /**
     * int bitmask of supported sfd ids
     * Values:
     *  HAS_SFD0_SUPPORT = 1
     *  HAS_SFD1_SUPPORT = (1 << 1)
     *  HAS_SFD2_SUPPORT = (1 << 2)
     *  HAS_SFD3_SUPPORT = (1 << 3)
     *  HAS_SFD4_SUPPORT = (1 << 4)
     */
    SUPPORTED_SFD_IDS = 0xD,
    /**
     * int bitmask of supported sts modes
     * Values:
     *  HAS_STATIC_STS_SUPPORT = 1
     *  HAS_DYNAMIC_STS_SUPPORT = (1 << 1)
     *  HAS_DYNAMIC_STS_INDIVIDUAL_CONTROLEE_KEY_SUPPORT = (1 << 2)
     */
    SUPPORTED_STS_MODES = 0xE,
    /**
     * int bitmask of supported sts segments
     * Values:
     *  HAS_0_SEGMENT_SUPPORT = 1
     *  HAS_1_SEGMENT_SUPPORT = (1 << 1)
     *  HAS_2_SEGMENT_SUPPORT = (1 << 2)
     */
    SUPPORTED_STS_SEGEMENTS = 0xF,
    /**
     * int bitmask of supported bprf phr data rates
     * Values:
     *  HAS_6M81_SUPPORT = 1
     *  HAS_850K_SUPPORT = (1 << 1)
     */
    SUPPORTED_BPRF_PHR_DATA_RATES = 0x10,
    /**
     * int bitmask of supported psdu data rates
     * Values:
     *  HAS_6M81_SUPPORT = 1
     *  HAS_7M80_SUPPORT = (1 << 1)
     *  HAS_27M2_SUPPORT = (1 << 2)
     *  HAS_31M2_SUPPORT = (1 << 3)
     */
    SUPPORTED_PSDU_DATA_RATES = 0x11,

    /*********************************************
     * CCC specific
     ********************************************/

    /**
     * 2 byte tuple {major_version (1 byte), minor_version (1 byte)} array with list of supported
     * CCC versions
     */
    CCC_SUPPORTED_VERSIONS = 0xA0,
    /**
     * byte array with a list of supported UWB configs
     * Values:
     *  UWB_CONFIG_0 = 0
     *  UWB_CONFIG_1 = 1
     */
    CCC_SUPPORTED_UWB_CONFIGS = 0xA1,
    /**
     * 1 byte tuple {initiator_tx (4 bits), responder_tx (4 bits)} array with list of supported
     * pulse shape combos
     * Values:
     *  PULSE_SHAPE_SYMMETRICAL_ROOT_RAISED_COSINE = 1
     *  PULSE_SHAPE_PRECURSOR_FREE = 2
     *  PULSE_SHAPE_PRECURSOR_FREE_SPECIAL = 3
     */
    /**  */
    CCC_SUPPORTED_PULSE_SHAPE_COMBOS = 0xA2,
    /** Int value for indicating supported ran multiplier */
    CCC_SUPPORTED_RAN_MULTIPLIER = 0xA3,
    /**
     * byte array with a list of supported chaps per slot
     * Values:
     *  CHAPS_PER_SLOT_3 = 3
     *  CHAPS_PER_SLOT_4 = 4
     *  CHAPS_PER_SLOT_6 = 6
     *  CHAPS_PER_SLOT_8 = 8
     *  CHAPS_PER_SLOT_9 = 9
     *  CHAPS_PER_SLOT_12 = 12
     *  CHAPS_PER_SLOT_24 = 24
     */
    CCC_SUPPORTED_CHAPS_PER_SLOT = 0xA4,
    /**
     * byte array with a list of supported sync codes
     * Values: 1 - 32
     */
    CCC_SUPPORTED_SYNC_CODES = 0xA5,
    /** byte array with list of supported channels */
    CCC_SUPPORTED_CHANNELS = 0xA6,
    /**
     * byte array with a list of supported hopping sequences
     * Values:
        HOPPING_SEQUENCE_DEFAULT = 0
        HOPPING_SEQUENCE_AES = 1
     */
    CCC_SUPPORTED_HOPPING_SEQUENCES = 0xA7,
    /**
     * byte array with a list of supported hopping config modes
     * Values:
     *  HOPPING_CONFIG_MODE_NONE = 0
     *  HOPPING_CONFIG_MODE_CONTINUOUS = 1
     *  HOPPING_CONFIG_MODE_ADAPTIVE = 2
     */
    CCC_SUPPORTED_HOPPING_CONFIG_MODES = 0xA8,
}
