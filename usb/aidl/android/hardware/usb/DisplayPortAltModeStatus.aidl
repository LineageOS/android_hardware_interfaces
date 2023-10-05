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
 * Port Partners:
 * Indicates the current status of the of DisplayPort Alt Mode
 * partner, which can either be a DisplayPort Alt Mode source
 * (ex. computer, tablet, phone) or DisplayPort Alt Mode sink
 * (monitor, hub, adapter).
 *
 * Cables:
 * Indicates the current status of the Type-C cable/adapter
 * connected to the local Android device.
 *
 */
enum DisplayPortAltModeStatus {
    /**
     * Port Partners:
     * The port partner status is currently unknown for one of
     * the following reasons:
     *     1. No port partner is connected to the device
     *     2. The USB Power Delivery Discover Identity command
     *        has not been issued to the port partner via SOP
     *        messaging.
     *
     * Cables:
     * The cable’s capabilities are not yet known to the
     * device, or no cable is plugged in.
     */
    UNKNOWN = 0,
    /**
     * Port Partners:
     * The current port partner does not list DisplayPort as
     * one of its Alt Modes,does not list the capability to
     * act as a DisplayPort Source or Sink device, or a compatible
     * configuration could not be established.
     *
     * Cables:
     * The cable/adapter’s capabilities do not list DisplayPort
     * as one of its Alt Modes, or a compatible configuration could
     * not be established.
     */
    NOT_CAPABLE = 1,
    /**
     * Port Partners:
     * The current port partner lists compatible DisplayPort
     * capabilities with the device, however may not yet have
     * entered DisplayPort Alt Mode or has configured its
     * port for data transmission.
     *
     * Cables:
     * The Type-C cable/adapter’s capabilities have been
     * discovered and list DisplayPort Alt Mode as one of its
     * capabilities, however may not yet have entered DisplayPort
     * Alt Mode or has been configured for data transmission.
     */
    CAPABLE = 2,
    /**
     * Port Partners:
     * The port partner and device are both configured for
     * DisplayPort Alt Mode.
     *
     * Cables:
     * The Type-C cable/adapter is configured for DisplayPort
     * Alt Mode.
     */
    ENABLED = 3,
}
