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
 * Wifi bands defined in the 80211 spec.
 */
@VintfStability
@Backing(type="int")
enum WifiBand {
    BAND_UNSPECIFIED = 0,
    /**
     * 2.4 GHz.
     */
    BAND_24GHZ = 1,
    /**
     * 5 GHz without DFS.
     */
    BAND_5GHZ = 2,
    /**
     * 5 GHz DFS only.
     */
    BAND_5GHZ_DFS = 4,
    /**
     * 5 GHz with DFS.
     */
    BAND_5GHZ_WITH_DFS = 6,
    /**
     * 2.4 GHz + 5 GHz; no DFS.
     */
    BAND_24GHZ_5GHZ = 3,
    /**
     * 2.4 GHz + 5 GHz with DFS.
     */
    BAND_24GHZ_5GHZ_WITH_DFS = 7,
    /**
     * 6 GHz.
     */
    BAND_6GHZ = 8,
    /**
     * 5 GHz no DFS + 6 GHz.
     */
    BAND_5GHZ_6GHZ = 10,
    /**
     * 2.4 GHz + 5 GHz no DFS + 6 GHz.
     */
    BAND_24GHZ_5GHZ_6GHZ = 11,
    /**
     * 2.4 GHz + 5 GHz with DFS + 6 GHz.
     */
    BAND_24GHZ_5GHZ_WITH_DFS_6GHZ = 15,
    /**
     * 60 GHz.
     */
    BAND_60GHZ = 16,
    /**
     * 2.4 GHz + 5 GHz no DFS + 6 GHz + 60 GHz.
     */
    BAND_24GHZ_5GHZ_6GHZ_60GHZ = 27,
    /**
     * 2.4 GHz + 5 GHz with DFS + 6 GHz + 60 GHz.
     */
    BAND_24GHZ_5GHZ_WITH_DFS_6GHZ_60GHZ = 31,
}
