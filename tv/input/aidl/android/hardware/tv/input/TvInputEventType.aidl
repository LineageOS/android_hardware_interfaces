/*
 * Copyright 2022 The Android Open Source Project
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

package android.hardware.tv.input;

@VintfStability
@Backing(type="int")
enum TvInputEventType {
    /**
     * Hardware notifies the framework that a device is available.
     *
     * Note that DEVICE_AVAILABLE and DEVICE_UNAVAILABLE events do not represent
     * hotplug events (i.e. plugging cable into or out of the physical port).
     * These events notify the framework whether the port is available or not.
     * For a concrete example, when a user plugs in or pulls out the HDMI cable
     * from a HDMI port, it does not generate DEVICE_AVAILABLE and/or
     * DEVICE_UNAVAILABLE events. However, if a user inserts a pluggable USB
     * tuner into the Android device, it must generate a DEVICE_AVAILABLE event
     * and when the port is removed, it must generate a DEVICE_UNAVAILABLE
     * event.
     *
     * For hotplug events, please see STREAM_CONFIGURATION_CHANGED for more
     * details.
     *
     * HAL implementation must register devices by using this event when the
     * device boots up. The framework must recognize device reported via this
     * event only.
     */
    DEVICE_AVAILABLE = 1,

    /**
     * Hardware notifies the framework that a device is unavailable.
     *
     * HAL implementation must generate this event when a device registered
     * by DEVICE_AVAILABLE is no longer available. For example,
     * the event can indicate that a USB tuner is plugged out from the Android
     * device.
     *
     * Note that this event is not for indicating cable plugged out of the port;
     * for that purpose, the implementation must use
     * STREAM_CONFIGURATION_CHANGED event. This event represents the port itself
     * being no longer available.
     */
    DEVICE_UNAVAILABLE = 2,

    /**
     * Stream configurations are changed. Client must regard all open streams
     * at the specific device are closed, and must call
     * getStreamConfigurations() again, opening some of them if necessary.
     *
     * HAL implementation must generate this event when the available stream
     * configurations change for any reason. A typical use case of this event
     * is to notify the framework that the input signal has changed resolution,
     * or that the cable is plugged out so that the number of available streams
     * is 0.
     *
     * The implementation must use this event to indicate hotplug status of the
     * port. the framework regards input devices with no available streams as
     * disconnected, so the implementation can generate this event with no
     * available streams to indicate that this device is disconnected, and vice
     * versa.
     */
    STREAM_CONFIGURATIONS_CHANGED = 3,
}
