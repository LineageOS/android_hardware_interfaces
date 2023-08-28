/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Copyright 2021 NXP.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.uwb;

import android.hardware.uwb.IUwbClientCallback;
import android.hardware.uwb.UwbStatus;

/**
 * Controls a UWB chip on the device. On some devices, there could be multiple UWB chips.
 */
@VintfStability
interface IUwbChip {
    /**
     * Get unique idenitifer for the chip.
     */
    String getName();

    /**
     * Performs the UWB HAL initialization and power on UWB Subsystem. If open completes
     * successfully, then UWB Subsystem is ready to accept UCI message through write() API
     *
     * @param clientCallback Client callback instance.
     */
    void open(in IUwbClientCallback clientCallback);

    /**
     * Close the UWB Subsystem. Should free all resources.
     */
    void close();

    /**
     * Perform UWB Subsystem initialization by applying all vendor configuration.
     */
    void coreInit();

    /**
     * Perform any necessary UWB session initializations.
     * This must be invoked by the framework at the beginging of every new ranging session.
     *
     * @param sessionId Session identifier as defined in the UCI specification.
     */
    void sessionInit(int sessionId);

    /**
     * Supported version of vendor UCI specification.
     *
     * @return Returns the version of the "android.hardware.uwb.fira_android" types-only
     * package included in the HAL implementation. This vendor params/commands package will be
     * updated on a different cadence to the main UWB HAL interface package.
     */
    int getSupportedAndroidUciVersion();

    /**
     * Write the UCI message to the UWB Subsystem.
     * The UCI message format is as per UCI  protocol and it is
     * defined in "FiRa Consortium - UCI Generic Specification_v1.0" specification at FiRa
     * consortium.
     *
     * UCI 1.1 specification: https://groups.firaconsortium.org/wg/members/document/1949.
     *
     * This method may queue writes and return immediately, or it may block until data is written.
     * Implementation must guarantee that writes are executed in order.
     *
     * @param data UCI packet to write.
     * @return number of bytes written to the UWB Subsystem
     */
    int sendUciMessage(in byte[] data);
}
