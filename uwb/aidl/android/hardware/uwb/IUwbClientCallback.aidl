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

import android.hardware.uwb.UwbEvent;
import android.hardware.uwb.UwbStatus;

@VintfStability
oneway interface IUwbClientCallback {
    /**
     * The callback passed in from the UWB stack that the HAL
     * can use to pass incoming data to the stack.  These include UCI
     * responses and notifications from the UWB subsystem.
     *
     * UCI 1.1 specification: https://groups.firaconsortium.org/wg/members/document/1949.
     *
     * @param data UCI packet sent.
     */
    void onUciMessage(in byte[] data);

    /**
     * The callback passed in from the UWB stack that the HAL
     * can use to pass events back to the stack.
     *
     * @param event Asynchronous event type.
     * @param status Associated status.
     */
    void onHalEvent(in UwbEvent event, in UwbStatus status);
}
