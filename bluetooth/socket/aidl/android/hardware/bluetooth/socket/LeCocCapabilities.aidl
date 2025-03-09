/*
 * Copyright 2024 The Android Open Source Project
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

package android.hardware.bluetooth.socket;

/**
 * Capabilities for LE L2CAP COC that the offload stack supports.
 */
@VintfStability
parcelable LeCocCapabilities {
    /**
     * Maximum number of LE COC sockets supported. If not supported, the value must be zero.
     */
    int numberOfSupportedSockets;

    /**
     * Local Maximum Transmission Unit size in octets. The MTU size must be in range 23 to 65535.
     *
     * The actual value of the local MTU shared in the connection configuration is set in
     * LeCocChannelInfo.localMtu in the IBluetoothSocket.opened() context parameter.
     */
    int mtu;

    /**
     * The value used by the Host stack for the local Maximum Packet Size shall be the value
     * LE_ACL_Data_Packet_Length returned by the controller in response to the command HCI LE Read
     * Buffer Size. Then, the MPS size must be in range 1 to 255. We do not make the MPS
     * configurable in HAL because using the maximum value does not require a large amount of
     * memory.
     */
}
