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
    /*********************************************
     * CCC specific
     ********************************************/
    UWB_CONFIG_0 = 0,
    UWB_CONFIG_1 = 1,

    PULSE_SHAPE_SYMMETRICAL_ROOT_RAISED_COSINE = 0,
    PULSE_SHAPE_PRECURSOR_FREE = 1,
    PULSE_SHAPE_PRECURSOR_FREE_SPECIAL = 2,

    CHAPS_PER_SLOT_3 = 1,
    CHAPS_PER_SLOT_4 = 1 << 1,
    CHAPS_PER_SLOT_6 = 1 << 2,
    CHAPS_PER_SLOT_8 = 1 << 3,
    CHAPS_PER_SLOT_9 = 1 << 4,
    CHAPS_PER_SLOT_12 = 1 << 5,
    CHAPS_PER_SLOT_24 = 1 << 6,

    HOPPING_SEQUENCE_DEFAULT = 1 << 4,
    HOPPING_SEQUENCE_AES = 1 << 3,

    HOPPING_CONFIG_MODE_NONE = 1 << 7,
    HOPPING_CONFIG_MODE_CONTINUOUS = 1 << 6,
    HOPPING_CONFIG_MODE_ADAPTIVE = 1 << 5,

    CCC_CHANNEL_5 = 1,
    CCC_CHANNEL_9 = 1 << 1,

    /*********************************************
     * RADAR specific
     ********************************************/
    RADAR_NOT_SUPPORTED = 0,
    RADAR_SWEEP_SAMPLES_SUPPORTED = 1,
}
