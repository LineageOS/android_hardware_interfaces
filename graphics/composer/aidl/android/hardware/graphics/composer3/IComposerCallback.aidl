/**
 * Copyright (c) 2021, The Android Open Source Project
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

package android.hardware.graphics.composer3;

import android.hardware.graphics.common.DisplayHotplugEvent;
import android.hardware.graphics.composer3.RefreshRateChangedDebugData;
import android.hardware.graphics.composer3.VsyncPeriodChangeTimeline;

@VintfStability
interface IComposerCallback {
    /**
     * Notifies the client that the given display has either been connected or
     * disconnected. Every active display (even a built-in physical display)
     * must trigger at least one hotplug notification, even if it only occurs
     * immediately after callback registration.
     *
     * Displays which have been connected are assumed to be in PowerMode.OFF,
     * and the onVsync callback should not be called for a display until vsync
     * has been enabled with setVsyncEnabled.
     *
     * The client may call back into the device while the callback is in
     * progress. The device must serialize calls to this callback such that
     * only one thread is calling it at a time.
     *
     * @param display is the display that triggers the hotplug event.
     * @param connected indicates whether the display is connected or
     *        disconnected.
     * @deprecated: Use instead onHotplugEvent
     */
    void onHotplug(long display, boolean connected);

    /**
     * Notifies the client to trigger a screen refresh. This forces all layer
     * state for this display to be resent, and the display to be validated
     * and presented, even if there have been no changes.
     *
     * This refresh will occur some time after the callback is initiated, but
     * not necessarily before it returns.  It is safe to trigger this callback
     * from other functions which call into the device.
     *
     * @param display is the display to refresh.
     */
    oneway void onRefresh(long display);

    /**
     * Notifies the client that the conditions which previously led to returning
     * SEAMLESS_NOT_POSSIBLE from setActiveConfigWithConstraints have changed and now seamless may
     * be possible. Client should retry calling setActiveConfigWithConstraints.
     *
     * @param display is a display setActiveConfigWithConstraints previously failed with
     * EX_SEAMLESS_NOT_POSSIBLE.
     */
    oneway void onSeamlessPossible(long display);

    /**
     * Notifies the client that a vsync event has occurred. This callback must
     * only be triggered when vsync is enabled for this display (through
     * setVsyncEnabled).
     *
     * @param display is the display which has received a vsync event
     * @param timestamp is the CLOCK_MONOTONIC time at which the vsync event
     *        occurred, in nanoseconds.
     * @param vsyncPeriodNanos is the display vsync period in nanoseconds i.e. the next onVsync
     *        is expected to be called vsyncPeriodNanos nanoseconds after this call.
     */
    oneway void onVsync(long display, long timestamp, int vsyncPeriodNanos);

    /**
     * Notifies the client that the previously reported timing for vsync period change has been
     * updated. This may occur if the composer missed the deadline for changing the vsync period
     * or the client submitted a refresh frame too late.
     *
     * @param display is the display which vsync period change is in progress
     * @param updatedTimeline is the new timeline for the vsync period change.
     */
    oneway void onVsyncPeriodTimingChanged(
            long display, in VsyncPeriodChangeTimeline updatedTimeline);

    /**
     * Notifies the client that the display is idle, the refresh rate changed to a lower setting to
     * preserve power and vsync cadence changed. When a new frame is queued for presentation, the
     * client is expected to enable vsync callbacks to learn the new vsync cadence before sending
     * a new frame.
     *
     * @param display is the display whose vsync cadence changed due to panel idle mode.
     */
    oneway void onVsyncIdle(long display);

    /**
     * Notifies the client the vsyncPeriod of the display changed.
     * Whether or not to call this callback is managed by
     * IComposerClient.setRefreshRateChangedCallbackDebugEnabled
     *
     * Immediate callback is required after the setRefreshRateChangedCallbackDebugEnabled
     * called.
     * When the panel refresh rate changes, as a result of a setActiveConfig or
     * setActiveConfigWithConstraints, this callback should be called with the new panel
     * refresh rate. In addition, when the panel refresh rate is changed by other means,
     * such as idleness or DOZE power state, this callback should be called as well.
     *
     * This callback is used for debug purposes, and not for scheduling frames,
     * therefore synchronization is not required.
     *
     * @see IComposerClient.setRefreshRateChangedCallbackDebugEnabled
     *
     * @param data is the data for the callback when refresh rate changed.
     */
    oneway void onRefreshRateChangedDebug(in RefreshRateChangedDebugData data);

    /**
     * Notifies the client that a DisplayHotplugEvent has occurred for the
     * given display. Every active display (even a built-in physical display)
     * must trigger at least one hotplug notification, even if it only occurs
     * immediately after callback registration.
     *
     * Displays which have been connected are assumed to be in PowerMode.OFF,
     * and the onVsync callback should not be called for a display until vsync
     * has been enabled with setVsyncEnabled.
     *
     * The client may call back into the device while the callback is in
     * progress. The device must serialize calls to this callback such that
     * only one thread is calling it at a time.
     *
     * @param display is the display that triggers the hotplug event.
     * @param event is the type of event that occurred.
     */
    void onHotplugEvent(long display, DisplayHotplugEvent event);
}
