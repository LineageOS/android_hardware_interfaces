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

import android.hardware.usb.PortMode;

@VintfStability
enum PortMode {
    /**
     * Indicates that the port does not have a mode.
     * In case of DRP, the current mode of the port is only resolved
     * when the type-c handshake happens.
     */
    NONE = 0,
    /**
     * Indicates that port can only act as device for data and sink for power.
     */
    UFP = 1,
    /**
     * Indicates the port can only act as host for data and source for power.
     */
    DFP = 2,
    /**
     * Indicates can either act as UFP or DFP at a given point of time.
     */
    DRP = 3,
    /*
     * Indicates that the port supports Audio Accessory mode.
     */
    AUDIO_ACCESSORY = 4,
    /*
     * Indicates that the port supports Debug Accessory mode.
     */
    DEBUG_ACCESSORY = 5,
}
