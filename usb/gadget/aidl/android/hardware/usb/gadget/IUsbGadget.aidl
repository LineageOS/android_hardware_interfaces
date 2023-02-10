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

import android.hardware.usb.gadget.IUsbGadgetCallback;

@VintfStability
oneway interface IUsbGadget {
    /**
     * This function is used to set the current USB gadget configuration.
     * Usb gadget needs to be reset if an USB configuration is already.
     *
     * @param functions The GadgetFunction bitmap. See GadgetFunction for
     *                  the value of each bit.
     * @param callback IUsbGadgetCallback::setCurrentUsbFunctionsCb used to
     *                 propagate back the status.
     * @param timeoutMs The maximum time (in milliseconds) within which the
     *                IUsbGadgetCallback needs to be returned.
     * @param transactionId ID to be used when invoking the callback.
     *
     */
    void setCurrentUsbFunctions(in long functions, in IUsbGadgetCallback callback,
            in long timeoutMs, long transactionId);

    /**
     * This function is used to query the USB functions included in the
     * current USB configuration.
     *
     * @param callback IUsbGadgetCallback::getCurrentUsbFunctionsCb used to
     *                 propagate the current functions list.
     * @param transactionId ID to be used when invoking the callback.
     */
    void getCurrentUsbFunctions(in IUsbGadgetCallback callback, long transactionId);

    /**
     * The function is used to query current USB speed.
     *
     * @param callback IUsbGadgetCallback::getUsbSpeedCb used to propagate
     *                 current USB speed.
     * @param transactionId ID to be used when invoking the callback.
     */
    void getUsbSpeed(in IUsbGadgetCallback callback, long transactionId);

    /**
     * This function is used to reset USB gadget driver.
     * Performs USB data connection reset. The connection will disconnect and
     * reconnect.
     *
     * @param callback IUsbGadgetCallback::resetCb used to propagate
     *                 the result of requesting resetUsbGadget.
     * @param transactionId ID to be used when invoking the callback.
     */
    void reset(in IUsbGadgetCallback callback, long transactionId);
}
