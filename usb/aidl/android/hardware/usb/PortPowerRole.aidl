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
enum PortPowerRole {
    /**
     * Indicates that the port does not have a power role.
     * In case of DRP, the current power role of the port is only resolved
     * when the type-c handshake happens.
     */
    NONE = 0,
    /**
     * Indicates that the port is supplying power to the other port.
     */
    SOURCE = 1,
    /**
     * Indicates that the port is sinking power from the other port.
     */
    SINK = 2,
}
