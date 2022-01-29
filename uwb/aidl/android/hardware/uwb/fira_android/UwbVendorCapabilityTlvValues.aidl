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
 * Android specific capability TLV values expected in UCI command:
 * GID: 0000b (UWB Core Group),
 * OID: 000011b (CORE_GET_CAPS_INFO_CMD),
 */
@VintfStability
@Backing(type="long")
enum UwbVendorCapabilityTlvValues {
    HAS_AZIMUTH_SUPPORT = 1,
    HAS_ELEVATION_SUPPORT = (1 << 1),
    HAS_FOM_SUPPORT = (1 << 2),
    HAS_FULL_AZIMUTH_SUPPORT = (1 << 3),

    HAS_CONTROLEE_INITIATOR_SUPPORT = 1,
    HAS_CONTROLEE_RESPONDER_SUPPORT = (1 << 1),
    HAS_CONTROLLER_INITIATOR_SUPPORT = (1 << 2),
    HAS_CONTROLLER_RESPONDER_SUPPORT = (1 << 3),

    HAS_CRC_16_SUPPORT = 1,
    HAS_CRC_32_SUPPORT = (1 << 1),

    HAS_UNICAST_SUPPORT = 1,
    HAS_ONE_TO_MANY_SUPPORT = (1 << 1),
    HAS_MANY_TO_MANY_SUPPORT = (1 << 2),

    SUPPORTED_PREAMBLE_HAS_32_SYMBOLS_SUPPORT = 1,
    SUPPORTED_PREAMBLE_HAS_64_SYMBOLS_SUPPORT = (1 << 1),

    HAS_BPRF_SUPPORT = 1,
    HAS_HPRF_SUPPORT = (1 << 1),

    HAS_DS_TWR_SUPPORT = 1,
    HAS_SS_TWR_SUPPORT = (1 << 1),

    HAS_SP0_RFRAME_SUPPORT = 1,
    HAS_SP1_RFRAME_SUPPORT = (1 << 1),
    HAS_SP3_RFRAME_SUPPORT = (1 << 3),

    HAS_SFD0_SUPPORT = 1,
    HAS_SFD1_SUPPORT = (1 << 1),
    HAS_SFD2_SUPPORT = (1 << 2),
    HAS_SFD3_SUPPORT = (1 << 3),
    HAS_SFD4_SUPPORT = (1 << 4),

    HAS_STATIC_STS_SUPPORT = 1,
    HAS_DYNAMIC_STS_SUPPORT = (1 << 1),
    HAS_DYNAMIC_STS_INDIVIDUAL_CONTROLEE_KEY_SUPPORT = (1 << 2),

    HAS_0_SEGMENT_SUPPORT = 1,
    HAS_1_SEGMENT_SUPPORT = (1 << 1),
    HAS_2_SEGMENT_SUPPORT = (1 << 2),

    HAS_6M81_SUPPORT = 1,
    HAS_850K_SUPPORT = (1 << 1),
    HAS_27M2_SUPPORT = (1 << 2),
    HAS_31M2_SUPPORT = (1 << 3),

    /*********************************************
     * CCC specific
     ********************************************/
    UWB_CONFIG_0 = 0,
    UWB_CONFIG_1 = 1,

    PULSE_SHAPE_SYMMETRICAL_ROOT_RAISED_COSINE = 1,
    PULSE_SHAPE_PRECURSOR_FREE = 2,
    PULSE_SHAPE_PRECURSOR_FREE_SPECIAL = 3,

    CHAPS_PER_SLOT_3 = 3,
    CHAPS_PER_SLOT_4 = 4,
    CHAPS_PER_SLOT_6 = 6,
    CHAPS_PER_SLOT_8 = 8,
    CHAPS_PER_SLOT_9 = 9,
    CHAPS_PER_SLOT_12 = 12,
    CHAPS_PER_SLOT_24 = 24,

    HOPPING_SEQUENCE_DEFAULT = 0,
    HOPPING_SEQUENCE_AES = 1,

    HOPPING_CONFIG_MODE_NONE = 0,
    HOPPING_CONFIG_MODE_CONTINUOUS = 1,
    HOPPING_CONFIG_MODE_ADAPTIVE = 2,
}
