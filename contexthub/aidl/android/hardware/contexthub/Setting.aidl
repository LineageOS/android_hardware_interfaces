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

package android.hardware.contexthub;

/**
 * Used to indicate the type of user setting that has changed.
 */
@VintfStability
@Backing(type="byte")
enum Setting {
    LOCATION = 1,
    /**
     * The main WiFi toggle in the Android settings for WiFi connectivity.
     */
    WIFI_MAIN,
    /**
     * The "Wi-Fi scanning" setting for location scans.
     */
    WIFI_SCANNING,
    AIRPLANE_MODE,
    /**
     * Indicates if the microphone access is available for CHRE. Microphone
     * access is disabled if the user has turned off the microphone as a
     * privacy setting, in which case audio data cannot be used and propagated
     * by CHRE.
     */
    MICROPHONE,
    /**
     * The main BT toggle in the Android settings for BT connectivity.
     */
    BT_MAIN,
    /**
     * The "BT scanning" setting for location scans.
     */
    BT_SCANNING,
}
