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

package android.hardware.wifi;

/**
 * Channel operating width in Mhz.
 */
@VintfStability
@Backing(type="int")
enum WifiChannelWidthInMhz {
    WIDTH_INVALID = -1,
    WIDTH_20 = 0,
    WIDTH_40 = 1,
    WIDTH_80 = 2,
    WIDTH_160 = 3,
    WIDTH_80P80 = 4,
    WIDTH_5 = 5,
    WIDTH_10 = 6,
    /**
     * 320 MHz
     */
    WIDTH_320 = 7,
}
