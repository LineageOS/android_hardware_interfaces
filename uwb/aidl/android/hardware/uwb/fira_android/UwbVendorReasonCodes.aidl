/*
 * Copyright (C) 2022 The Android Open Source Project
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
 * Android specific vendor reason codes should be defined here.
 *
 */
@VintfStability
@Backing(type="int")
enum UwbVendorReasonCodes {
    /**
     * Use values from the vendor specific reason code range: 0x80 – 0xFF defined in
     * Table 15 (state change with reason codes) of UCI specification.
     */

    /** Fira specific */
    /** The channel requested is not available for AoA */
    REASON_ERROR_INVALID_CHANNEL_WITH_AOA = 0x80,
    /** UWB stopped caused by other session conflict */
    REASON_ERROR_STOPPED_DUE_TO_OTHER_SESSION_CONFLICT = 0x81,
    /** UWB has been disabled (eg: country code change leads to UWB unsupported) */
    REASON_REGULATION_UWB_OFF = 0x82,
}
