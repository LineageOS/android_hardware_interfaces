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
 * Android specific vendor app config values set/expected in UCI command:
 * GID: 0001b (UWB Session config Group)
 * OID: 000011b (SESSION_SET_APP_CONFIG_CMD)
 * OID: 000100b (SESSION_GET_APP_CONFIG_CMD)
 *
 * Note: Refer to Table 34 of the UCI specification for the other values
 * expected in this command.
 */
@VintfStability
@Backing(type="int")
enum UwbVendorSessionAppConfigTlvValues {
    /**
     * Added in vendor version 0.
     * Supported only if the UwbVendorCapabilityTlvTypes
     * .SUPPORTED_AOA_RESULT_REQ_ANTENNA_INTERLEAVING set to 1.
     * Set AOA_RESULT_REQ (Config ID - 0x0D) to this value to turn on antenna
     * interleaving feature.
     */
    AOA_RESULT_REQ_ANTENNA_INTERLEAVING = 0xF0,
}
