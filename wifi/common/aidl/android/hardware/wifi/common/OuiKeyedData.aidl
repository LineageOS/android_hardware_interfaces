/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.wifi.common;

import android.os.PersistableBundle;

/**
 * Data for OUI-based configuration.
 */
@VintfStability
parcelable OuiKeyedData {
    /**
     * OUI : 24-bit organizationally unique identifier to identify the vendor/OEM.
     * See https://standards-oui.ieee.org/ for more information.
     */
    int oui;
    /**
     * Vendor data. Expected fields should be defined by the vendor.
     */
    PersistableBundle vendorData;
}
