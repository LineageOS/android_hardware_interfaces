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

@VintfStability
enum UsbDataStatus {
    /**
     * USB data status not known.
     */
    UNKNOWN = 0,
    /**
     * USB data is enabled.
     */
    ENABLED = 1,
    /**
     * USB data is disabled as the port is hot.
     */
    DISABLED_OVERHEAT = 2,
    /**
     * USB data is disabled as port is contaminated.
     */
    DISABLED_CONTAMINANT = 3,
    /**
     * USB data is disabled due to dock.
     */
    DISABLED_DOCK = 4,
    /**
     * USB data is disabled by USB Service.
     */
    DISABLED_FORCE = 5,
    /**
     * USB data disabled for debug.
     */
    DISABLED_DEBUG = 6
}
