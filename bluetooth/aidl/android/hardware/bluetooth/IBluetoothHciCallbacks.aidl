/*
 * Copyright 2022 The Android Open Source Project
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

package android.hardware.bluetooth;

import android.hardware.bluetooth.Status;

/**
 * The interface from the Bluetooth Controller to the stack.
 */
@VintfStability
interface IBluetoothHciCallbacks {
    /**
     * Send an ACL data packet from the controller to the host.
     * @param data the ACL HCI packet to be passed to the host stack
     */
    void aclDataReceived(in byte[] data);

    /**
     * This function is invoked when an HCI event is received from the
     * Bluetooth controller to be forwarded to the Bluetooth stack.
     * @param event is the HCI event to be sent to the Bluetooth stack.
     */
    void hciEventReceived(in byte[] event);

    /**
     * Invoked when the Bluetooth controller initialization has been
     * completed.
     * @param status contains a return code indicating success, or the
     *               reason the initialization failed.
     */
    void initializationComplete(in Status status);

    /**
     * Send a ISO data packet from the controller to the host.
     * @param data the ISO HCI packet to be passed to the host stack
     */
    void isoDataReceived(in byte[] data);

    /**
     * Send a SCO data packet from the controller to the host.
     * @param data the SCO HCI packet to be passed to the host stack
     */
    void scoDataReceived(in byte[] data);
}
