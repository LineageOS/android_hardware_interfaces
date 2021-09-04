/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.radio;

/**
 * Contains the configuration of the LTE cell tower.
 */
@VintfStability
parcelable CellConfigLte {
    /**
     * Indicates that if E-UTRA-NR Dual Connectivity (EN-DC) is supported by the LTE cell.
     * True if the plmn-InfoList-r15 is present in SIB2 and at least one bit in this list is true,
     * otherwise this value should be false.
     * Reference: 3GPP TS 36.331 v15.2.2 6.3.1 System information blocks.
     */
    boolean isEndcAvailable;
}
