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

import android.hardware.bluetooth.socket.ChannelInfo;
import android.hardware.contexthub.EndpointId;

/**
 * Socket context.
 */
@VintfStability
parcelable SocketContext {
    /**
     * Identifier assigned to the socket by the host stack when the socket is connected to a remote
     * device. Used to uniquely identify the socket in other callbacks and method invocations. It is
     * valid only while the socket is connected.
     */
    long socketId;

    /**
     * Descriptive socket name provided by the host app when it creates this socket. This is not
     * unique across the system, but can help the offload app understand the purpose of the socket
     * when it receives a socket connection event.
     */
    String name;

    /**
     * ACL connection handle for the socket.
     */
    int aclConnectionHandle;

    /**
     * Channel information of the socket protocol.
     */
    ChannelInfo channelInfo;

    /**
     * Unique identifier for an endpoint at the hardware offload data path.
     */
    EndpointId endpointId;
}
