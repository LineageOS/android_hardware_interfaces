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

import android.hardware.bluetooth.gatt.Uuid;

@VintfStability
/**
 * Represents a GATT characteristic.
 */
parcelable GattCharacteristic {
    /**
     * Characteristic UUID for attribute value.
     */
    Uuid uuid;

    /**
     * Bit mask of characteristic properties, defining the allowed operations.
     *
     * This field represents the permitted properties of a Bluetooth Low Energy (BLE)
     * characteristic, encoded as a bit mask. This mask must be a subset of all attribute properties
     * enabled by the local or remote GATT server. Each bit corresponds to a specific characteristic
     * property flag, as defined in the Bluetooth Core Specification Version 6.0, Volume 3, Part G,
     * Section 3.3.1.1.
     *
     * The following bits indicate the allowed operations for the endpoints:
     * 0x02: Read
     * 0x04: Write Without Response
     * 0x08: Write
     * 0x10: Notify
     * 0x20: Indicate
     *
     * If a client attempts an operation that is not permitted by these properties,
     * the GATT server may respond with an ATT_ERROR_RSP.
     */
    int properties;

    /**
     * Attribute handle of characteristic.
     *
     * The range of valid values is 0x0001-0xffff.
     */
    int valueHandle;
}
