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

package android.hardware.wifi.supplicant;

/**
 * WPS config methods.
 * Refer to section 3 of IBSS with Wi-Fi Protected Setup
 * Technical Specification Version 1.0.0.
 */
@VintfStability
@Backing(type="int")
enum WpsConfigMethods {
    USBA = 0x0001,
    ETHERNET = 0x0002,
    LABEL = 0x0004,
    DISPLAY = 0x0008,
    EXT_NFC_TOKEN = 0x0010,
    INT_NFC_TOKEN = 0x0020,
    NFC_INTERFACE = 0x0040,
    PUSHBUTTON = 0x0080,
    KEYPAD = 0x0100,
    VIRT_PUSHBUTTON = 0x0280,
    PHY_PUSHBUTTON = 0x0480,
    P2PS = 0x1000,
    VIRT_DISPLAY = 0x2008,
    PHY_DISPLAY = 0x4008,
}
