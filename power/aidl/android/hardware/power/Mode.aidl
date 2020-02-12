/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.power;

@VintfStability
@Backing(type="int")
enum Mode {
    /**
     * This mode indicates that the device is to allow wake up when the
     * screen is tapped twice.
     */
    DOUBLE_TAP_TO_WAKE,

    /**
     * This mode indicates Low power mode is activated or not. Low power
     * mode is intended to save battery at the cost of performance.
     */
    LOW_POWER,

    /**
     * This mode indicates Sustained Performance mode is activated or not.
     * Sustained performance mode is intended to provide a consistent level of
     * performance for a prolonged amount of time.
     */
    SUSTAINED_PERFORMANCE,

    /**
     * This mode indicates VR Mode is activated or not. VR mode is intended
     * to provide minimum guarantee for performance for the amount of time the
     * device can sustain it.
     */
    VR,

    /**
     * This mode indicates that an application has been launched.
     */
    LAUNCH,

    /**
     * This mode indicates that the device is about to enter a period of
     * expensive rendering.
     */
    EXPENSIVE_RENDERING,

    /**
     * This mode indicates that the device is about entering/leaving
     * interactive state. (that is, the system is awake and ready for
     * interaction, often with UI devices such as display and touchscreen
     * enabled) or non-interactive state (the
     * system appears asleep, display usually turned off). The
     * non-interactive state may be entered after a period of
     * inactivity in order to conserve battery power during
     * such inactive periods.
     *
     * Typical actions are to turn on or off devices and adjust
     * cpufreq parameters. This function may also call the
     * appropriate interfaces to allow the kernel to suspend the
     * system to low-power sleep state when entering non-interactive
     * state, and to disallow low-power suspend when the system is in
     * interactive state. When low-power suspend state is allowed, the
     * kernel may suspend the system whenever no wakelocks are held.
     */
    INTERACTIVE,

    /**
     * This mode indicates the device is in device idle, externally known as doze.
     * More details on:
     * https://developer.android.com/training/monitoring-device-state/doze-standby
     */
    DEVICE_IDLE,

    /**
     * This mode indicates that display is either off or still on but is optimized
     * for low-power.
     */
    DISPLAY_INACTIVE,

    /**
     * Below hints are currently not sent in Android framework but OEM might choose to
     * implement for power/perf optimizations.
     */

    /**
     * This mode indicates that low latency audio is active.
     */
    AUDIO_STREAMING_LOW_LATENCY,

    /**
     * This hint indicates that camera secure stream is being started.
     */
    CAMERA_STREAMING_SECURE,

    /**
     * This hint indicates that camera low resolution stream is being started.
     */
    CAMERA_STREAMING_LOW,

    /**
     * This hint indicates that camera mid resolution stream is being started.
     */
    CAMERA_STREAMING_MID,

    /**
     * This hint indicates that camera high resolution stream is being started.
     */
    CAMERA_STREAMING_HIGH,
}
