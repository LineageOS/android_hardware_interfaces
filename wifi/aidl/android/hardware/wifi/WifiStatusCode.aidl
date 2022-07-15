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
 * Enum values indicating the status of the WiFi operation.
 */
@VintfStability
@Backing(type="int")
enum WifiStatusCode {
    /**
     * No errors.
     */
    SUCCESS,
    /**
     * Method invoked on an invalid |IWifiChip| object.
     */
    ERROR_WIFI_CHIP_INVALID,
    /**
     * Method invoked on an invalid |IWifiIface| object.
     */
    ERROR_WIFI_IFACE_INVALID,
    /**
     * Method invoked on an invalid |IWifiRttController| object.
     */
    ERROR_WIFI_RTT_CONTROLLER_INVALID,
    ERROR_NOT_SUPPORTED,
    ERROR_NOT_AVAILABLE,
    ERROR_NOT_STARTED,
    ERROR_INVALID_ARGS,
    ERROR_BUSY,
    ERROR_UNKNOWN,
}
