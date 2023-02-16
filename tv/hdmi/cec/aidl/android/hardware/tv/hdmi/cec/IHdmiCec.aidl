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

package android.hardware.tv.hdmi.cec;

import android.hardware.tv.hdmi.cec.CecLogicalAddress;
import android.hardware.tv.hdmi.cec.CecMessage;
import android.hardware.tv.hdmi.cec.IHdmiCecCallback;
import android.hardware.tv.hdmi.cec.Result;
import android.hardware.tv.hdmi.cec.SendMessageResult;

/**
 * HDMI-CEC HAL interface definition.
 */
@VintfStability
interface IHdmiCec {
    /**
     * Passes the logical address that must be used in this system.
     *
     * HAL must use it to configure the hardware so that the CEC commands
     * addressed the given logical address can be filtered in. This method must
     * be able to be called as many times as necessary in order to support
     * multiple logical devices.
     *
     * @param addr Logical address that must be used in this system. It must be
     *        in the range of valid logical addresses for the call to succeed.
     * @return Result status of the operation. SUCCESS if successful,
     *         FAILURE_INVALID_ARGS if the given logical address is invalid,
     *         FAILURE_BUSY if device or resource is busy
     */
    Result addLogicalAddress(in CecLogicalAddress addr);

    /**
     * Clears all the logical addresses.
     *
     * It is used when the system doesn't need to process CEC command any more,
     * hence to tell HAL to stop receiving commands from the CEC bus, and change
     * the state back to the beginning.
     */
    void clearLogicalAddress();

    /**
     * Configures ARC circuit in the hardware logic to start or stop the
     * feature.
     *
     * @param portId Port id to be configured.
     * @param enable Flag must be either true to start the feature or false to
     *        stop it.
     */
    void enableAudioReturnChannel(in int portId, in boolean enable);

    /**
     * Returns the CEC version supported by underlying hardware.
     *
     * @return the CEC version supported by underlying hardware.
     */
    int getCecVersion();

    /**
     * Gets the CEC physical address.
     *
     * The physical address depends on the topology of the network formed by
     * connected HDMI devices. It is therefore likely to change if the cable is
     * plugged off and on again. It is advised to call getPhysicalAddress to get
     * the updated address when hot plug event takes place.
     *
     * @return Physical address of this device.
     */
    int getPhysicalAddress();

    /**
     * Gets the identifier of the vendor.
     *
     * @return Identifier of the vendor that is the 24-bit unique
     *         company ID obtained from the IEEE Registration Authority
     *         Committee (RAC). The upper 8 bits must be 0.
     */
    int getVendorId();

    /**
     * Transmits HDMI-CEC message to other HDMI device.
     *
     * The method must be designed to return in a certain amount of time and not
     * hanging forever which may happen if CEC signal line is pulled low for
     * some reason.
     *
     * It must try retransmission at least once as specified in the section '7.1
     * Frame Re-transmissions' of the CEC Spec 1.4b.
     *
     * @param message CEC message to be sent to other HDMI device.
     * @return Result status of the operation. SUCCESS if successful,
     *         NACK if the sent message is not acknowledged,
     *         BUSY if the CEC bus is busy,
     *         FAIL if the message could not be sent.
     */
    SendMessageResult sendMessage(in CecMessage message);

    /**
     * Sets a callback that HDMI-CEC HAL must later use for incoming CEC
     * messages.
     *
     * @param callback Callback object to pass hdmi events to the system. The
     *        previously registered callback must be replaced with this one.
     *        setCallback(null) should deregister the callback.
     */
    void setCallback(in @nullable IHdmiCecCallback callback);

    /**
     * Passes the updated language information of Android system. Contains
     * three-letter code as defined in ISO/FDIS 639-2. Must be used for HAL to
     * respond to <Get Menu Language> while in standby mode.
     *
     * @param language Three-letter code defined in ISO/FDIS 639-2. Must be
     *        lowercase letters. (e.g., eng for English)
     */
    void setLanguage(in String language);

    /**
     * Determines whether a TV panel device in standby mode should wake up when
     * it receives an OTP (One Touch Play) from a source device.
     *
     * @param value If true, the TV device will wake up when OTP is received
     *              and if false, the TV device will not wake up for an OTP.
     */
    void enableWakeupByOtp(in boolean value);

    /**
     * Switch to enable or disable CEC on the device.
     *
     * @param value If true, the device will have all CEC functionalities
     *              and if false, the device will not perform any CEC functions.
     */
    void enableCec(in boolean value);

    /**
     * Determines which module processes CEC messages - the Android framework or
     * the HAL.
     *
     * @param value If true, the Android framework will actively process CEC
     *        messages and if false, only the HAL will process the CEC messages.
     */
    void enableSystemCecControl(in boolean value);
}
