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

package android.hardware.usb;

import android.hardware.usb.IUsbCallback;
import android.hardware.usb.PortRole;

@VintfStability
oneway interface IUsb {
    /**
     * When supportsEnableContaminantPresenceDetection is true,
     * enableContaminantPresenceDetection enables/disables contaminant
     * presence detection algorithm. Calling enableContaminantPresenceDetection
     * when supportsEnableContaminantPresenceDetection is false does
     * not have any effect.
     * Change in contantaminant presence status should be notified to the
     * client via notifyPortStatusChange through PortStatus.
     *
     * @param portName name of the port.
     * @param enable true Enable contaminant presence detection algorithm.
     *               false Disable contaminant presence detection algorithm.
     * @param transactionId ID to be used when invoking the callback.
     */
    void enableContaminantPresenceDetection(in String portName, in boolean enable, long transactionId);

    /**
     * This function is used to enable/disable USB data controller.
     *
     * @param portName Name of the port.
     * @param enable   true Enable USB data signaling.
     *                 false Disable USB data signaling.
     * @param transactionId ID to be used when invoking the callback.
     *
     */
    void enableUsbData(in String portName, boolean enable, long transactionId);

    /**
     * This function is used to enable USB controller if and when the controller
     * disabled due to docking event.
     *
     * @param portName Name of the port.
     * @param transactionId ID to be used when invoking the callback.
     */
    void enableUsbDataWhileDocked(in String portName, long transactionId);

    /**
     * This functions is used to request the hal for the current status
     * status of the Type-C ports. The result of the query would be sent
     * through the IUsbCallback object's notifyRoleSwitchStatus
     * to the caller. This api would would let the caller know of the number
     * of type-c ports that are present and their connection status through the
     * PortStatus type.
     * @param transactionId ID to be used when invoking the callback.
     */
    void queryPortStatus(long transactionId);

    /**
     * This function is used to register a callback function which is
     * called by the HAL to inform the client of port status updates and
     * result of the requested operation. Please refer IUsbCallback for
     * complete description of when each of the IUsbCallback's interface
     * methods is expected to be called.
     *
     * @param callback IUsbCallback object used to convey status to the
     * userspace.
     */
    void setCallback(in IUsbCallback callback);

    /**
     * This function is used to change the port role of a specific port.
     * For example, when DR_SWAP or PR_SWAP is supported.
     * The status of the role switch will be informed through IUsbCallback
     * object's notifyPortStatusChange method.
     *
     * @param portName name of the port for which the role has to be changed
     * @param role the new port role.
     * @param transactionId ID to be used when invoking the callback.
     */
    void switchRole(in String portName, in PortRole role, long transactionId);

    /**
     * This function is used to limit power transfer in and out of the port.
     * When limited, the port does not charge from the partner port.
     * Also, the port limits sourcing power to the partner port when the USB
     * specification allows it to do so.
     *
     * @param portName name of the port for which power transfer is being limited.
     * @param limit true limit power transfer.
     *              false relax limiting power transfer.
     * @param transactionId ID to be used when invoking the callback.
     */
    void limitPowerTransfer(in String portName, boolean limit, long transactionId);

    /**
     * This function is used to reset the port role of a specific port.
     * For instance, when data transfer through the port fails.
     *
     * @param portName name of the port that is being reset
     * @param transactionId ID to be used when invoking the callback.
     */
    void resetUsbPort(in String portName, long transactionId);
}
