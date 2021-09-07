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
 * Size limits for some of the params used in this interface.
 */
@VintfStability
@Backing(type="int")
enum ParamSizeLimits {
    /**
     * Max length of SSID param.
     */
    SSID_MAX_LEN_IN_BYTES = 32,
    /**
     * Min length of PSK passphrase param.
     */
    WPA2_PSK_PASSPHRASE_MIN_LEN_IN_BYTES = 8,
    /**
     * Max length of PSK passphrase param.
     */
    WPA2_PSK_PASSPHRASE_MAX_LEN_IN_BYTES = 63,
}
