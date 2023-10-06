/*
 * Copyright (C) 2023 The Android Open Source Project
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
 * P2P Client IPV4 address allocated via EAPOL exchange.
 * The IP addresses are IPV4 addresses and higher-order address bytes are in the lower-order
 * int bytes (e.g. 1.2.3.4 is represented as 0x04030201)
 */
@VintfStability
parcelable P2pClientEapolIpAddressInfo {
    /**
     * The P2P Client IP address.
     */
    int ipAddressClient;
    /**
     * The subnet that the P2P Group Owner is using.
     */
    int ipAddressMask;
    /**
     * The P2P Group Owner IP address.
     */
    int ipAddressGo;
}
