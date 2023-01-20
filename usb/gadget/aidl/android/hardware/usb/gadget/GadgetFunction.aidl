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

@VintfStability
parcelable GadgetFunction {
    /**
     * Removes all the functions and pulls down the gadget.
     */
    const long NONE = 0;
    /**
     * Android Debug Bridge function.
     */
    const long ADB = 1;
    /**
     * Android open accessory protocol function.
     */
    const long ACCESSORY = 1 << 1;
    /**
     * Media Transfer protocol function.
     */
    const long MTP = 1 << 2;
    /**
     * Peripheral mode USB Midi function.
     */
    const long MIDI = 1 << 3;
    /**
     * Picture transfer protocol function.
     */
    const long PTP = 1 << 4;
    /**
     * Tethering function.
     */
    const long RNDIS = 1 << 5;
    /**
     * AOAv2.0 - Audio Source function.
     */
    const long AUDIO_SOURCE = 1 << 6;
    /**
     * UVC - Universal Video Class function.
     */
    const long UVC = 1 << 7;
    /**
     * NCM - NCM function.
     */
    const long NCM = 1 << 10;
}
