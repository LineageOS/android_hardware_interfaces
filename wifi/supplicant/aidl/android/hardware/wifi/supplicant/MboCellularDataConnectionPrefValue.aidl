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

package android.hardware.wifi.supplicant;

/**
 *  MBO spec v1.2, 4.2.5 Table 16: MBO Cellular Data connection preference
 *  attribute values. AP use this to indicate STA, its preference for the
 *  STA to move from BSS to cellular network.
 */
@VintfStability
@Backing(type="int")
enum MboCellularDataConnectionPrefValue {
    EXCLUDED = 0,
    NOT_PREFERRED = 1,
    /*
     * 2-254 Reserved.
     */
    PREFERRED = 255,
}
