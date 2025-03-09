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

import android.hardware.bluetooth.socket.IBluetoothSocketCallback;
import android.hardware.bluetooth.socket.SocketCapabilities;
import android.hardware.bluetooth.socket.SocketContext;

/**
 * The interface for host stack to register callback, get capabilities, and open/close socket.
 */
@VintfStability
interface IBluetoothSocket {
    /**
     * API to register a callback for HAL implementation to send asynchronous events.
     *
     * @param callback An instance of the |IBluetoothSocketCallback| AIDL interface object
     */
    void registerCallback(in IBluetoothSocketCallback callback);

    /**
     * API to get supported offload socket capabilities.
     *
     * @return a socket capabilities
     */
    SocketCapabilities getSocketCapabilities();

    /**
     * API to notify the offload stack that the socket is opened.
     *
     * The HAL implementation must use IBluetoothSocketCallback.openedComplete() to indicate the
     * result of this operation
     *
     * @param context Socket context including socket id, channel, hub, and endpoint info
     */
    void opened(in SocketContext context);

    /**
     * API to notify the offload stack that the socket is closed.
     *
     * When host app requests to close a socket or the HAL calls IBluetoothSocketCallback.close(),
     * the host stack closes the socket and sends the notification.
     *
     * @param socketId Identifier assigned to the socket by the host stack
     */
    void closed(long socketId);
}
