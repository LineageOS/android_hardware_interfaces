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
enum ContaminantProtectionMode {
    /**
     * No action performed upon detection of contaminant presence.
     */
    NONE = 0,
    /**
     * Upon detection of contaminant presence, Port is forced to sink only
     * mode where a port shall only detect chargers until contaminant presence
     * is no longer detected.
     */
    FORCE_SINK = 1,
    /**
     * Upon detection of contaminant presence, Port is forced to source only
     * mode where a port shall only detect usb accessories such as headsets
     * until contaminant presence is no longer detected.
     */
    FORCE_SOURCE = 2,
    /**
     * Upon detection of contaminant presence, port is disabled until contaminant
     * presence is no longer detected. In the disabled state port will
     * not respond to connection of chargers or usb accessories.
     */
    FORCE_DISABLE = 3,
}
