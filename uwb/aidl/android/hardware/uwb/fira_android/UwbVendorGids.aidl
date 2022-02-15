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
 * Android specific vendor command GIDs (Group ID) should be defined here.
 *
 * For each vendor GID defined here, also create a corresponding AIDL file enumerating the
 * OIDs (Opcode Identifier) allowed within that GID.
 * For ex: VENDOR_GID_XXX = 1110b -> UwbVendorGidXXXOids.aidl
 */
@VintfStability
@Backing(type="byte")
enum UwbVendorGids {
    /**
     * Use values from the Proprietary Group range: 1110b – 1111b defined in Table 36 of
     * UCI specification.
     */

    /** All Android specific commands/response/notification should use this GID */
    ANDROID = 0xC,
}
