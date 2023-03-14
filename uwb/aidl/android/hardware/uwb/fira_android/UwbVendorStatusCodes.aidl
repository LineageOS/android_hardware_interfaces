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
 * Android specific vendor status codes should be defined here.
 *
 */
@VintfStability
@Backing(type="byte")
enum UwbVendorStatusCodes {
    /**
     * Use values from the vendor specific status code range: 0x50 – 0xFF defined in Table 32 of
     * UCI specification.
     */

    /** CCC specific */
    /** Secure element is busy */
    STATUS_ERROR_CCC_SE_BUSY = 0x50,
    /** CCC Lifecycle error */
    STATUS_ERROR_CCC_LIFECYCLE = 0x51,
    /** Other session conflict */
    STATUS_ERROR_STOPPED_DUE_TO_OTHER_SESSION_CONFLICT = 0x52,
    /** UWB Regulation Off */
    STATUS_REGULATION_UWB_OFF = 0x53,
}
