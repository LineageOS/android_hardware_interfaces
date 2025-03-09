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

import android.hardware.bluetooth.socket.Status;

/**
 * The interface from the Bluetooth offload socket to the host stack.
 */
@VintfStability
interface IBluetoothSocketCallback {
    /**
     * Invoked when IBluetoothSocket.opened() has been completed.
     *
     * @param socketId Identifier assigned to the socket by the host stack
     * @param status Status indicating success or failure
     * @param reason Reason string of the operation failure for debugging purposes
     */
    void openedComplete(long socketId, in Status status, in String reason);

    /**
     * Invoked when offload app or stack requests host stack to close the socket.
     *
     * @param socketId Identifier assigned to the socket by the host stack
     * @param reason Reason string for closing the socket for debugging purposes
     */
    void close(long socketId, in String reason);
}
