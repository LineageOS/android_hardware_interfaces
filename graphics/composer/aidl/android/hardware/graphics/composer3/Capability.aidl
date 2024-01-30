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

/**
 * Optional capabilities which may be supported by some devices. The
 * particular set of supported capabilities for a given device may be
 * retrieved using getCapabilities.
 */
@VintfStability
@Backing(type="int")
enum Capability {
    INVALID = 0,
    /**
     * Specifies that the device supports sideband stream layers, for
     * which buffer content updates and other synchronization will not be
     * provided through the usual validate/present cycle and must be
     * handled by an external implementation-defined mechanism. Only
     * changes to layer state (such as position, size, etc.) need to be
     * performed through the validate/present cycle.
     */
    SIDEBAND_STREAM = 1,
    /**
     * Specifies that the device will apply a color transform even when
     * either the client or the device has chosen that all layers should
     * be composed by the client. This will prevent the client from
     * applying the color transform during its composition step.
     */
    SKIP_CLIENT_COLOR_TRANSFORM = 2,
    /**
     * Specifies that the present fence must not be used as an accurate
     * representation of the actual present time of a frame.
     */
    PRESENT_FENCE_IS_NOT_RELIABLE = 3,
    /**
     * Specifies that a device is able to skip the validateDisplay call before
     * receiving a call to presentDisplay. The client will always skip
     * validateDisplay and try to call presentDisplay regardless of the changes
     * in the properties of the layers. If the device returns anything else than
     * no error, it will call validateDisplay then presentDisplay again.
     * For this capability to be worthwhile the device implementation of
     * presentDisplay should fail as fast as possible in the case a
     * validateDisplay step is needed.
     * @deprecated - enabled by default.
     */
    SKIP_VALIDATE = 4,

    /**
     * Specifies that the device supports setting a display configuration that
     * the device should boot at.
     * @see IComposerClient.setBootDisplayConfig
     * @see IComposerClient.clearBootDisplayConfig
     * @see IComposerClient.getPreferredBootDisplayConfig
     */
    BOOT_DISPLAY_CONFIG = 5,

    /**
     * Specifies that the device supports HDR output conversion.
     *
     * @see IComposerClient.getHdrConversionCapabilities
     * @see IComposerClient.setHdrConversionStrategy
     */
    HDR_OUTPUT_CONVERSION_CONFIG = 6,

    /**
     * Specifies that the device supports the callback onRefreshRateChangedDebug
     * to pass information about the refresh rate.
     * The refresh rate from the callback is used to update the refresh rate
     * overlay indicator.
     *
     * @see IComposerClient.setRefreshRateChangedCallbackDebugEnabled
     * @see IComposerCallback.onRefreshRateChangedDebug
     */
    REFRESH_RATE_CHANGED_CALLBACK_DEBUG = 7,

    /**
     * Specifies that the device HAL supports the batching of layer creation and destruction
     * for better performance.
     *
     * @see IComposerClient.executeCommands
     * @see LayerCommand.layerLifecycleBatchCommandType
     * @see LayerCommand.newBufferSlotCount
     */
    LAYER_LIFECYCLE_BATCH_COMMAND = 8,
}
