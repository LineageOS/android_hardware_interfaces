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
 * Possible composition types for a given layer.
 */
@VintfStability
@Backing(type="int")
enum Composition {
    INVALID = 0,
    /**
     * The client must composite this layer into the client target buffer
     * (provided to the device through setClientTarget).
     *
     * The device must not request any composition type changes for layers
     * of this type.
     */
    CLIENT = 1,
    /**
     * The device must handle the composition of this layer through a
     * hardware overlay or other similar means.
     *
     * Upon validateDisplay, the device may request a change from this
     * type to CLIENT.
     */
    DEVICE = 2,
    /**
     * The device must render this layer using the color set through
     * setLayerColor. If this functionality is not supported on a layer
     * that the client sets to SOLID_COLOR, the device must request that
     * the composition type of that layer is changed to CLIENT upon the
     * next call to validateDisplay.
     *
     * Upon validateDisplay, the device may request a change from this
     * type to CLIENT.
     */
    SOLID_COLOR = 3,
    /**
     * Similar to DEVICE, but the position of this layer may also be set
     * asynchronously through setCursorPosition. If this functionality is
     * not supported on a layer that the client sets to CURSOR, the device
     * must request that the composition type of that layer is changed to
     * CLIENT upon the next call to validateDisplay.
     *
     * Upon validateDisplay, the device may request a change from this
     * type to either DEVICE or CLIENT.  Changing to DEVICE will prevent
     * the use of setCursorPosition but still permit the device to
     * composite the layer.
     */
    CURSOR = 4,
    /**
     * The device must handle the composition of this layer, as well as
     * its buffer updates and content synchronization. Only supported on
     * devices which provide Capability.SIDEBAND_STREAM.
     *
     * Upon validateDisplay, the device may request a change from this
     * type to either DEVICE or CLIENT, but it is unlikely that content
     * will display correctly in these cases.
     */
    SIDEBAND = 5,
    /**
     * A display decoration layer contains a buffer which is used to provide
     * anti-aliasing on the cutout region and rounded corners on the top and
     * bottom of a display.
     *
     * Only supported if the device returns a valid struct from
     * getDisplayDecorationSupport. Pixels in the buffer are interpreted
     * according to the DisplayDecorationSupport.alphaInterpretation.
     *
     * Upon validateDisplay, the device may request a change from this type
     * to either DEVICE or CLIENT.
     */
    DISPLAY_DECORATION = 6,

    /**
     * This composition type is similar to DEVICE, with a single difference,
     * that indicates to HWC that this layer update is not considered an activity
     * of any sort. For example, If HWC maintains a timer for activity to switch
     * the display mode from a power save mode, it should not reset that timer.
     *
     * Upon validateDisplay, the device may request a change from this type to CLIENT.
     */
    REFRESH_RATE_INDICATOR = 7,
}
