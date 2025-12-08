/*
 * Copyright (C) 2025 The Android Open Source Project
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
 * Specifies the network type for UI display purposes, as indicated by the modem.
 * This value dictates the primary iconography and branding shown to the user.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum DisplayNetworkType {
    /**
     * Indicates modem does not know or suggest any display network type.
     */
    UNKNOWN = 0,

    /**
     * Indicates modem suggests device to show the 5G advanced icon
     * (e.g., "5G+", "5G UW", "5G UC").
     */
    NR_ADVANCED = 1,
}
