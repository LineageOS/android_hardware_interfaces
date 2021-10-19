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
 * Android specific vendor app params set in UCI command:
 * GID: 0001b (UWB Session config Group)
 * OID: 000011b (SESSION_SET_APP_CONFIG_CMD)
 *
 * Note: Refer to Table 34 of the UCI specification for the other params
 * expected in this command.
 */
@VintfStability
@Backing(type="int")
enum UwbVendorSessionSetAppConfigCmdParams {
    /** CCC params for ranging start */

    /** Added in vendor version 0. */
    CCC_RANGING_PROTOCOL_VER = 0xA3,
    CCC_UWB_CONFIG_ID = 0xA4,
    CCC_PULSESHAPE_COMBO = 0xA5,
    CCC_URSK_TTL = 0xA6,
}
