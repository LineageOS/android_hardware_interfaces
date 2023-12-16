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
 * Enum describing the various states to set the roaming
 * control to.
 */
@VintfStability
@Backing(type="byte")
enum StaRoamingState {
    /**
     * Driver/Firmware must not perform any roaming.
     */
    DISABLED = 0,
    /**
     * Driver/Firmware is allowed to perform roaming while respecting
     * the |StaRoamingConfig| parameters set using |configureRoaming|.
     */
    ENABLED = 1,
    /**
     * Driver/Firmware is allowed to roam more aggressively. For instance,
     * roaming can be triggered at higher RSSI thresholds than normal.
     */
    AGGRESSIVE = 2,
}
