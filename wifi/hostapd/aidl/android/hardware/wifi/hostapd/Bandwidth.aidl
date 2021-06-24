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

package android.hardware.wifi.hostapd;

/**
 * The channel bandwidth of the AP.
 */
@VintfStability
@Backing(type="int")
enum Bandwidth {
    BANDWIDTH_INVALID = 0,
    BANDWIDTH_20_NOHT = 1,
    BANDWIDTH_20 = 2,
    BANDWIDTH_40 = 3,
    BANDWIDTH_80 = 4,
    BANDWIDTH_80P80 = 5,
    BANDWIDTH_160 = 6,
    BANDWIDTH_2160 = 7,
    BANDWIDTH_4320 = 8,
    BANDWIDTH_6480 = 9,
    BANDWIDTH_8640 = 10,
}
