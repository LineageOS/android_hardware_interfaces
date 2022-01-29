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
 * Android specific vendor app params set/expected in UCI command:
 * GID: 0001b (UWB Session config Group)
 * OID: 000011b (SESSION_SET_APP_CONFIG_CMD)
 * OID: 000100b (SESSION_GET_APP_CONFIG_CMD)
 *
 * Note: Refer to Table 34 of the UCI specification for the other params
 * expected in this command.
 */
@VintfStability
@Backing(type="int")
enum UwbVendorSessionAppConfigTlvTypes {
    /** CCC params for ranging start */

    /**
     * Added in vendor version 0.
     * Range 0xA0 - 0xDF reserved for CCC use.
     */
    /** 16 byte data */
    CCC_HOP_MODE_KEY = 0xA0,
    /** 8 byte data */
    CCC_UWB_TIME0 = 0xA1,
    /** 2 byte data */
    CCC_RANGING_PROTOCOL_VER = 0xA3,
    /** 2 byte data */
    CCC_UWB_CONFIG_ID = 0xA4,
    /** 1 byte data */
    CCC_PULSESHAPE_COMBO = 0xA5,
    /** 2 byte data */
    CCC_URSK_TTL = 0xA6,

    /**
     * Range 0xE3 - 0xFF is reserved for proprietary use in the FIRA spec.
     * In Android, this range is reserved by the Android platform for Android-specific
     * app config cmd params.
     * Vendors must not define additional app config cmd params in this range.
     */

    /**
     * Added in vendor version 0.
     * Interleaving ratio if AOA_RESULT_REQ is set to 0xF0.
     * Supported only if the value returned by getSupportedAndroidCapabilities()
     * has the bit of UwbAndroidCapabilities.ANTENNAE_INTERLEAVING set to 1.
     */
    /** 2 byte data */
    NB_OF_RANGE_MEASUREMENTS = 0xE3,
    /** 2 byte data */
    NB_OF_AZIMUTH_MEASUREMENTS = 0xE4,
    /** 2 byte data */
    NB_OF_ELEVATION_MEASUREMENTS = 0xE5,
}
