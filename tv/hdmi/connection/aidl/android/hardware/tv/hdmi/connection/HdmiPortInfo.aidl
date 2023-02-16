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

package android.hardware.tv.hdmi.connection;

import android.hardware.tv.hdmi.connection.HdmiPortType;

/**
 * HDMI port descriptor
 */
@VintfStability
parcelable HdmiPortInfo {
    HdmiPortType type;
    int portId; // For devices with input ports (e.g. TV Panels), input ports should start from 1
                // which corresponds to HDMI "port 1".

    // In the following, 'supported' refers to having the necessary hardware and firmware on the
    // device to support CEC/ARC/eARC on this port.
    boolean cecSupported;
    boolean arcSupported; // If true, cecSupported has to be true as well. ARC cannot be supported
                          // without CEC support.
    boolean eArcSupported;

    // The physical address of the device connected to this port, valid range is 0x0000 to 0xFFFF
    // (ref Sec 8.7.2 of HDMI 1.4b).
    int physicalAddress;
}
