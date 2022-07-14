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
 * Interface operating modes.
 */
@VintfStability
@Backing(type="int")
enum WifiIfaceMode {
    /**
     * Interface operation mode is client.
     */
    IFACE_MODE_STA = 1 << 0,
    /**
     * Interface operation mode is Hotspot.
     */
    IFACE_MODE_SOFTAP = 1 << 1,
    /**
     * Interface operation mode is Ad-Hoc network.
     */
    IFACE_MODE_IBSS = 1 << 2,
    /**
     * Interface operation mode is Wifi Direct Client.
     */
    IFACE_MODE_P2P_CLIENT = 1 << 3,
    /**
     * Interface operation mode is Wifi Direct Group Owner.
     */
    IFACE_MODE_P2P_GO = 1 << 4,
    /**
     * Interface operation mode is Aware.
     */
    IFACE_MODE_NAN = 1 << 5,
    /**
     * Interface operation mode is Mesh network.
     */
    IFACE_MODE_MESH = 1 << 6,
    /**
     * Interface operation mode is Tunneled Direct Link Setup.
     */
    IFACE_MODE_TDLS = 1 << 7,
}
