/*
 * Copyright (C) 2025 The Android Open Source Project
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

package android.hardware.bluetooth.gatt;

@VintfStability
parcelable GattCapabilities {
    /**
     * The supported properties of characteristic which contains a bit mask of property flags
     * indicating the features of this characteristic by GATT client.
     *
     * This field represents the supported properties of a Bluetooth Low Energy (BLE)
     * characteristic, encoded as a bit mask. Each bit corresponds to a specific
     * characteristic property flag, as defined in the Bluetooth Core Specification
     * Version 6.0, Volume 3, Part G, Section 3.3.1.1.
     *
     * If GATT client offload is supported, the Notify bit (0x10) must be set. Other properties are
     * optional.
     */
    int supportedGattClientProperties;

    /**
     * The supported properties of characteristic which contains a bit mask of property flags
     * indicating the features of this characteristic by GATT server.
     *
     * This field represents the supported properties of a Bluetooth Low Energy (BLE)
     * characteristic, encoded as a bit mask. Each bit corresponds to a specific
     * characteristic property flag, as defined in the Bluetooth Core Specification
     * Version 6.0, Volume 3, Part G, Section 3.3.1.1.
     *
     * If GATT server offload is supported, the Notify bit (0x10) must be set. Other properties are
     * optional.
     */
    int supportedGattServerProperties;
}
