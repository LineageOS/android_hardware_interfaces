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
 * WPS Device Password ID
 */
@VintfStability
@Backing(type="int")
enum WpsDevPasswordId {
    DEFAULT = 0x0000,
    USER_SPECIFIED = 0x0001,
    MACHINE_SPECIFIED = 0x0002,
    REKEY = 0x0003,
    PUSHBUTTON = 0x0004,
    REGISTRAR_SPECIFIED = 0x0005,
    NFC_CONNECTION_HANDOVER = 0x0007,
    P2PS_DEFAULT = 0x0008,
}
