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

import android.hardware.usb.PortRole;
import android.hardware.usb.PortStatus;
import android.hardware.usb.Status;

/**
 * Callback object used for all the IUsb async methods which expects a result.
 * Caller is expected to register the callback object using setCallback method
 * to receive updates on the PortStatus.
 */
@VintfStability
oneway interface IUsbCallback {
    /**
     * Used to convey the current port status to the caller.
     * Must be called either when PortState changes due to the port partner or
     * when caller requested for the PortStatus update through queryPortStatus.
     *
     * @param currentPortStatus describes the status of all the USB ports in the
     *                          device.
     * @param retval SUCCESS when the required information was enquired form
     *               kernel and the PortStatus object was built.
     *               ERROR otherwise.
     */
    void notifyPortStatusChange(in PortStatus[] currentPortStatus, in Status retval);

    /**
     * Used to notify the result of the switchRole call to the caller.
     *
     * @param portName name of the port for which the roleswap is requested.
     * @param newRole the new role requested by the caller.
     * @param retval SUCCESS if the role switch succeeded. FAILURE otherwise.
     * @param transactionId  transactionId sent during switchRole request.
     */
    void notifyRoleSwitchStatus(in String portName, in PortRole newRole, in Status retval,
            long transactionId);

    /**
     * Used to notify the result of notifyEnableUsbDataStatus call to the caller.
     *
     * @param portName name of the port for which the enableUsbData is requested.
     * @param enable true when usb data is enabled.
     *               false when usb data is disabled.
     * @param retval SUCCESS if current request succeeded. FAILURE otherwise.
     * @param transactionId transactionId sent during enableUsbData request.
     */
    void notifyEnableUsbDataStatus(in String portName, boolean enable, in Status retval,
            long transactionId);

    /**
     * Used to notify the result of enableUsbDataWhileDocked call to the caller.
     *
     * @param portName name of the port for which the enableUsbDataWhileDocked is requested.
     * @param retval SUCCESS if current request succeeded. FAILURE otherwise.
     * @param transactionId transactionId sent during enableUsbDataWhileDocked request.
     */
    void notifyEnableUsbDataWhileDockedStatus(in String portName, in Status retval,
            long transactionId);

    /**
     * Used to notify the result of enableContaminantPresenceDetection.
     *
     * @param portName name of the port for which contaminant detection is enabled/disabled.
     * @param enable true when contaminant detection is enabled.
     *               false when disabled.
     * @param retval SUCCESS if the request for enabling/disabling contamiant detection succeeds.
     *               FAILURE otherwise.
     * @param transactionId transactionId sent during queryPortStatus request
     */
    void notifyContaminantEnabledStatus(in String portName, boolean enable, in Status retval,
            long transactionId);

    /**
     * Used to notify the request to query port status.
     *
     * @param portName name of the port for which port status is queried.
     * @param retval SUCCESS if the port query succeeded. FAILURE otherwise.
     * @param transactionId transactionId sent during queryPortStatus request
     */
    void notifyQueryPortStatus(in String portName, in Status retval, long transactionId);

    /**
     * Used to notify the result of requesting limitPowerTransfer.
     *
     * @param portName name of the port for which power transfer is being limited.
     * @param limit true limit power transfer.
     *              false relax limiting power transfer.
     * @param retval SUCCESS if the request to enable/disable limitPowerTransfer succeeds.
     *               FAILURE otherwise.
     * @param transactionId ID sent during limitPowerTransfer request.
     */
    void notifyLimitPowerTransferStatus(in String portName, boolean limit, in Status retval, long transactionId);

    /**
     * Used to notify the result of requesting resetUsbPort.
     *
     * @param portName name of the port that was being reset.
     * @param retval SUCCESS if current request succeeded. FAILURE otherwise.
     * @param transactionId current transactionId sent during resetUsbPort request.
     */
    void notifyResetUsbPortStatus(in String portName, in Status retval, long transactionId);
}
