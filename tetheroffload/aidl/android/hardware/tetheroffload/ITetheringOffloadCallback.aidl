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

package android.hardware.tetheroffload;

import android.hardware.tetheroffload.NatTimeoutUpdate;
import android.hardware.tetheroffload.OffloadCallbackEvent;

/**
 * Callback providing information about status of hardware management process
 * as well as providing a way to keep offloaded connections from timing out.
 */
@VintfStability
oneway interface ITetheringOffloadCallback {
    /**
     * Called when an asynchronous event is generated by the hardware
     * management process.
     */
    void onEvent(in OffloadCallbackEvent event);

    /**
     *  Provide a way for the management process to request that a connections
     *  timeout be updated in kernel.
     *
     *  This is necessary to ensure that offloaded traffic is not cleaned up
     *  by the kernel connection tracking module for IPv4.
     */
    void updateTimeout(in NatTimeoutUpdate params);
}