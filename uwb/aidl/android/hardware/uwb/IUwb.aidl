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
 * HAL Interface for UWB (Ultrawideband) subsystem.
 * https://en.wikipedia.org/wiki/Ultra-wideband.
 */
@VintfStability
interface IUwb {
    /**
     * Close the UWB Subsystem. Should free all resources.
     *
     */
    void close();

    /**
     * Perform UWB Subsystem initialization by applying all vendor configuration.
     *
     */
    void coreInit();

    /**
     * Performs the UWB HAL initialization and power on UWB Subsystem. If open completes
     * successfully, then UWB Subsystem is ready to accept UCI message through write() API
     *
     * @param clientCallback Client callback instance.
     */
    void open(in IUwbClientCallback clientCallback);

    /**
     * Write the UCI message to the UWB Subsystem.
     * The UCI message format is as per UCI  protocol and it is
     * defined in "FiRa Consortium - UCI Generic Specification_v1.0" specification at FiRa
     * consortium.
     * WIP doc link: https://groups.firaconsortium.org/wg/Technical/document/folder/127.
     * TODO(b/196004116): Link to the published specification.
     *
     * This method may queue writes and return immediately, or it may block until data is written.
     * Implementation must guarantee that writes are executed in order.
     *
     * @param data UCI packet to write.
     * @return number of bytes written to the UWB Subsystem
     */
    int write(in byte[] data);
}
