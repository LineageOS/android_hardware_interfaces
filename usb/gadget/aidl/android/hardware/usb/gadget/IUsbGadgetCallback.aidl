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

package android.hardware.usb.gadget;

import android.hardware.usb.gadget.Status;
import android.hardware.usb.gadget.UsbSpeed;

@VintfStability
oneway interface IUsbGadgetCallback {
    /**
     * Callback function used to propagate the status of configuration
     * switch to the caller.
     *
     * @param functions list of functions defined by GadgetFunction
     *                  included in the current USB gadget composition.
     * @param status SUCCESS when the functions are applied.
     *               FUNCTIONS_NOT_SUPPORTED when the configuration is
     *                                       not supported.
     *               ERROR otherwise.
     * @param transactionId ID to be used when invoking the callback.
     */
    void setCurrentUsbFunctionsCb(in long functions, in Status status, long transactionId);

    /**
     * Callback function used to propagate the current USB gadget
     * configuration.
     * @param functions The GadgetFunction bitmap. See GadgetFunction for
     *                  the value of each bit.
     * @param status FUNCTIONS_APPLIED when list of functions have been
     *                                 applied.
     *               FUNCTIONS_NOT_APPLIED when the functions have not
     *                                     been applied.
     *               ERROR otherwise.
     * @param transactionId ID to be used when invoking the callback.
     */
    void getCurrentUsbFunctionsCb(in long functions, in Status status, long transactionId);

    /**
     * Used to convey the current USB speed to the caller.
     * Must be called either when USB state changes due to USB enumeration or
     * when caller requested for USB speed through getUsbSpeed.
     *
     * @param speed USB Speed defined by UsbSpeed showed current USB
     *              connection speed.
     * @param transactionId ID to be used when invoking the callback.
     */
    void getUsbSpeedCb(in UsbSpeed speed, long transactionId);

    /**
     * Callback function used to propagate the result of requesting
     * resetUsbGadget.
     * @param status SUCCESS if current request succeeded. FAILURE otherwise.
     * @param transactionId current transactionId sent during reset request.
     */
    void resetCb(in Status status, long transactionId);
}
