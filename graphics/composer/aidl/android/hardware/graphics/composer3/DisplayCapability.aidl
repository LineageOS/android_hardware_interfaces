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

import android.hardware.graphics.composer3.DisplayCapability;

/**
 * Required capabilities which are supported by the display. The
 * particular set of supported capabilities for a given display may be
 * retrieved using getDisplayCapabilities.
 */
@VintfStability
@Backing(type="int")
enum DisplayCapability {
    INVALID = 0,
    /**
     * Indicates that the display must apply a color transform even when
     * either the client or the device has chosen that all layers should
     * be composed by the client. This prevents the client from applying
     * the color transform during its composition step.
     * If getDisplayCapabilities is supported, the global capability
     * SKIP_CLIENT_COLOR_TRANSFORM is ignored.
     * If getDisplayCapabilities is not supported, and the global capability
     * SKIP_CLIENT_COLOR_TRANSFORM is returned by getCapabilities,
     * then all displays must be treated as having
     * SKIP_CLIENT_COLOR_TRANSFORM.
     */
    SKIP_CLIENT_COLOR_TRANSFORM = 1,
    /**
     * Indicates that the display supports PowerMode.DOZE and
     * potentially PowerMode.DOZE_SUSPEND if DisplayCapability.SUSPEND is also
     * supported. DOZE_SUSPEND may not provide any benefit
     * over DOZE (see the definition of PowerMode for more information),
     * but if both DOZE and DOZE_SUSPEND are no different from
     * PowerMode.ON, the device must not claim support.
     * Must be returned by getDisplayCapabilities when getDozeSupport
     * indicates the display supports PowerMode.DOZE and
     * PowerMode.DOZE_SUSPEND.
     */
    DOZE = 2,
    /**
     * Indicates that the display supports brightness operations.
     */
    BRIGHTNESS = 3,
    /**
     * Indicates that the display supports protected contents.
     * When returned, hardware composer must be able to accept client target
     * with protected buffers.
     */
    PROTECTED_CONTENTS = 4,
    /**
     * Indicates that both the composer HAL implementation and the given display
     * support a low latency mode, such as HDMI 2.1 Auto Low Latency Mode.
     */
    AUTO_LOW_LATENCY_MODE = 5,
    /**
     * Indicates that the display supports PowerMode.ON_SUSPEND.
     * If PowerMode.ON_SUSPEND is no different from PowerMode.ON, the device must not
     * claim support.
     * If the display supports DisplayCapability.DOZE and DisplayCapability.SUSPEND, then
     * PowerMode.ON_SUSPEND and PowerMode.DOZE_SUSPEND must be supported.
     */
    SUSPEND = 6,
    /**
     * Indicates that the display supports IComposerClient.setIdleTimerEnabled and
     * IComposerCallback.onVsyncIdle.
     */
    DISPLAY_IDLE_TIMER = 7,
    /**
     * Indicates that both the composer HAL implementation and the given display
     * support calling executeCommands concurrently from separate threads.
     * executeCommands for a particular display will never run concurrently to
     * any other executeCommands for the same display. In addition, the
     * CommandResultPayload must only reference displays included in the
     * DisplayCommands passed to executeCommands. Displays referenced from
     * separate threads must have minimal interference with one another. If a
     * HWC-managed display has this capability, SurfaceFlinger can run
     * executeCommands for this display concurrently with other displays with the
     * same capability.
     * @see IComposerClient.executeCommands
     * @see DisplayCommand.presentDisplay
     * @see DisplayCommand.validateDisplay
     */
    MULTI_THREADED_PRESENT = 8,
}
