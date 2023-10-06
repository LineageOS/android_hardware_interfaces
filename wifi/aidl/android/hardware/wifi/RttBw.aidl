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
 * RTT Measurement Bandwidth.
 */
@VintfStability
@Backing(type="int")
enum RttBw {
    BW_UNSPECIFIED = 0x0,
    BW_5MHZ = 0x01,
    BW_10MHZ = 0x02,
    BW_20MHZ = 0x04,
    BW_40MHZ = 0x08,
    BW_80MHZ = 0x10,
    BW_160MHZ = 0x20,
    BW_320MHZ = 0x40,
}
