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
 * Detailed network mode for legacy network
 */
@VintfStability
@Backing(type="int")
enum LegacyMode {
    UNKNOWN = 0,
    /**
     * For 802.11a
     */
    A_MODE = 1,
    /**
     * For 802.11b
     */
    B_MODE = 2,
    /**
     * For 802.11g
     */
    G_MODE = 3,
}
