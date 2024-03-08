/*
 * Copyright 2023 The Android Open Source Project
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

package android.hardware.bluetooth.audio;

@VintfStability
union CodecId {
    /**
     * Codec Identifier defined for A2DP
     * The values are assigned by BT Sig [Assigned Numbers - 6.5.1]
     */
    enum A2dp { SBC = 0, AAC = 2 }

    /**
     * Codec Identifier defined for the Bluetooth Core Specification
     * The values are assigned by BT Sig [Assigned Numbers - 2.11]
     */
    enum Core { CVSD = 2, MSBC = 5, LC3 = 6 }

    /**
     * Vendor Codec:
     * id       16 bits - Assigned by BT Sig
     * codecId  16 bits - Assigned by the vendor
     */
    parcelable Vendor {
        int id;
        int codecId;
    }

    /**
     * Standard (A2DP or Core numbering space) or vendor
     */
    A2dp a2dp = A2dp.SBC;
    Core core;
    Vendor vendor;
}
