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
     * Sets the device to a fixed performance level which can be sustained under
     * normal indoor conditions for at least 10 minutes.
     *
     * This is similar to sustained performance mode, except that whereas
     * sustained performance mode puts an upper bound on performance in the
     * interest of long-term stability, fixed performance mode puts both upper
     * and lower bounds on performance such that any workload run while in a
     * fixed performance mode should complete in a repeatable amount of time
     * (except if the device is under thermal throttling).
     *
     * This mode is not intended for general purpose use, but rather to enable
     * games and other performance-sensitive applications to reduce the number
     * of variables during profiling and performance debugging. As such, while
     * it is valid to set the device to minimum clocks for all subsystems in
     * this mode, it is preferable to attempt to make the relative performance
     * of the CPU, GPU, and other subsystems match typical usage, even if the
     * frequencies have to be reduced to provide sustainability.
     *
     * To calibrate this mode, follow these steps:
     *
     * 1) Build and push the HWUI macrobench as described in
     *    //frameworks/base/libs/hwui/tests/macrobench/how_to_run.txt
     * 2) Run the macrobench as follows:
     *    while true; do \
     *      adb shell /data/benchmarktest/hwuimacro/hwuimacro shadowgrid2 -c 200 -r 10; \
     *    done
     * 3) Determine a fixed set of device clocks such that the loop in (2) can
     *    run for at least 10 minutes, starting from an idle device on a desk
     *    at room temperature (roughly 22 Celsius), without hitting thermal
     *    throttling.
     * 4) After setting those clocks, set the system property
     *    ro.power.fixed_performance_scale_factor to a value N, where N is the
     *    number of times the loop from (2) runs during the 10 minute test
     *    cycle. It is expected that in FIXED_PERFORMANCE mode, unless there is
     *    thermal throttling, the loop will run N to N+1 times (inclusive).
     *
     * After calibrating this, while in FIXED_PERFORMANCE mode, the macrobench
     * results obtained while running the loop in (2) should be consistent both
     * within a given run and from the first run in the 10 minute window through
     * the last run in the window.
     */
    FIXED_PERFORMANCE,

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
