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

package android.hardware.usb;

@VintfStability
@Backing(type="int")
/**
 * Indicates DisplayPort Alt Mode pin assignments whose port
 * pin configurations are as defined by DisplayPort Alt Mode
 * v1.0 for deprecated pin assignments A, B, and F and
 * DisplayPort Alt Mode v2.x for pin assignments C, D, and E.
 *
 */
enum DisplayPortAltModePinAssignment {
    /**
     * Indicates that the pin assignment has not yet been
     * configured, the attached cable/adapter does not support
     * DisplayPort Alt Mode, or no cable/adapter is attached.
     */
    NONE = 0,
    /**
     * Intended for use with USB-C-to-USB-C cables and with
     * adapters from USB-C to other video formats using
     * four lanes for DisplayPort transmission, and is
     * restricted by the USB Type-C r1.0 Active Cable
     * definition.
     */
    A = 1,
    /**
     * Intended for use with USB-C-to-USB-C cables and with
     * adapters from USB-C to other video formats using
     * two lanes for DisplayPort transmission and two for
     * USB SuperSpeed,and is restricted by the USB Type-C
     * r1.0 Active Cable definition.
     */
    B = 2,
    /**
     * Intended for use with USB-C-to-USB-C cables and with
     * adapters from USB-C to other video formats using
     * four lanes for DisplayPort transmission.
     */
    C = 3,
    /**
     * Intended for use with USB-C-to-USB-C cables and with
     * adapters from USB-C to other video formats using
     * two lanes for DisplayPort transmission and two for
     * USB SuperSpeed.
     */
    D = 4,
    /**
     * Intended for use with adapters from USB-C-to-DP plugs
     * or receptacles.
     */
    E = 5,
    /**
     * Intended for use with adapters from USB-C-to-DP plugs
     * or receptacles that also support two lanes of USB
     * SuperSpeed.
     */
    F = 6,
}
