/**
 * Copyright (c) 2023, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.graphics.common;

/**
 * Display hotplug events through onHotplugEvent callback.
 */
@VintfStability
@Backing(type="int")
enum DisplayHotplugEvent {
    /**
     * Display was successfully connected.
     * CONNECTED may be emitted more than once and the behavior of subsequent
     * events is that SurfaceFlinger queries the display properties again.
     */
    CONNECTED = 0,

    /** Display was successfully disconnected */
    DISCONNECTED = 1,

    /** Unknown error occurred */
    ERROR_UNKNOWN = -1,

    /** Display was plugged in, but incompatible cable error detected */
    ERROR_INCOMPATIBLE_CABLE = -2,

    /**
     * Display was plugged in, but exceeds the max number of
     * displays that can be simultaneously connected
     */
    ERROR_TOO_MANY_DISPLAYS = -3,

    /**
     * Display link is unstable, e.g. link training failure (negotiation
     * of connection speed failed), and the display needs to be
     * reconfigured
     */
    ERROR_LINK_UNSTABLE = -4,
}
