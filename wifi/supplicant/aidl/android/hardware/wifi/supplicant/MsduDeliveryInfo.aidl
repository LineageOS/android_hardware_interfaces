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

package android.hardware.wifi.supplicant;

/**
 * MSDU delivery information.
 * See Section 9.4.2.316 of the IEEE P802.11be/D4.0 Standard.
 */
@VintfStability
parcelable MsduDeliveryInfo {
    /**
     * Enums for the |deliveryRatio| field.
     * See Table 9-404t of the IEEE P802.11be/D4.0 Standard.
     */
    @VintfStability
    @Backing(type="byte")
    enum DeliveryRatio {
        RATIO_95 = 1, // 95%
        RATIO_96 = 2, // 96%
        RATIO_97 = 3, // 97%
        RATIO_98 = 4, // 98%
        RATIO_99 = 5, // 99%
        RATIO_99_9 = 6, // 99.9%
        RATIO_99_99 = 7, // 99.99%
        RATIO_99_999 = 8, // 99.999%
        RATIO_99_9999 = 9, // 99.9999%
    }

    /**
     * Percentage of the MSDUs that are expected to be delivered successfully.
     */
    DeliveryRatio deliveryRatio;

    /**
     * Exponent from which the number of incoming MSDUs is computed. The number of incoming
     * MSDUs is 10^countExponent, and is used to determine the MSDU delivery ratio.
     * Must be a number between 0 and 15 (inclusive).
     */
    byte countExponent;
}
