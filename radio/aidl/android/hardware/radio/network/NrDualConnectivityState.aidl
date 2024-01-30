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

package android.hardware.radio.network;

/**
 * NR Dual connectivity state
 * @hide
 */
@VintfStability
@Backing(type="byte")
@JavaDerive(toString=true)
enum NrDualConnectivityState {
    /**
     * Enable NR dual connectivity. Enabled state does not mean dual connectivity is active.
     * It means device is allowed to connect to both primary and secondary.
     */
    ENABLE = 1,
    /**
     * Disable NR dual connectivity. Disabled state does not mean secondary cell is released.
     * Modem will release it only if current bearer is released to avoid radio link failure.
     */
    DISABLE = 2,
    /**
     * Disable NR dual connectivity and force secondary cell to be released if dual connectivity
     * was active. This may result in radio link failure.
     */
    DISABLE_IMMEDIATE = 3,
}
