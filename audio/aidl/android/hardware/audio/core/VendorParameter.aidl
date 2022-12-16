/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.audio.core;

/**
 * Vendor parameters are used as a lightweight way to pass vendor-specific
 * configuration data back and forth between the HAL and vendor's extension
 * to the Android framework, without the need to extend audio interfaces
 * from AOSP.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable VendorParameter {
    /**
     * Vendor-generated unique ID of the parameter. In order to avoid
     * collisions, vendors must use a vendor-specific prefix for parameter
     * ids. The Android framework always passes ids as-is, without any attempt
     * to parse their content.
     */
    @utf8InCpp String id;
    /**
     * The payload of the parameter.
     */
    ParcelableHolder ext;
}
