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
