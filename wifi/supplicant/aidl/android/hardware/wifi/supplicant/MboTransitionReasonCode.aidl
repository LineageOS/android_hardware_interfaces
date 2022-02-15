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
 *  MBO spec v1.2, 4.2.6 Table 18: MBO transition reason code attribute
 *  values.
 */
@VintfStability
@Backing(type="byte")
enum MboTransitionReasonCode {
    UNSPECIFIED = 0,
    EXCESSIVE_FRAME_LOSS = 1,
    EXCESSIVE_TRAFFIC_DELAY = 2,
    INSUFFICIENT_BANDWIDTH = 3,
    LOAD_BALANCING = 4,
    LOW_RSSI = 5,
    RX_EXCESSIVE_RETRIES = 6,
    HIGH_INTERFERENCE = 7,
    GRAY_ZONE = 8,
    TRANSITION_TO_PREMIUM_AP = 9,
}
