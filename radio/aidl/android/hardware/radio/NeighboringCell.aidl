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

@VintfStability
parcelable NeighboringCell {
    /**
     * Combination of LAC and cell ID in 32 bits in GSM. Upper 16 bits is LAC and lower 16 bits is
     * CID (as described in TS 27.005).
     */
    String cid;
    /**
     * Received RSSI in GSM, level index of CPICH Received Signal Code Power in UMTS
     */
    int rssi;
}
