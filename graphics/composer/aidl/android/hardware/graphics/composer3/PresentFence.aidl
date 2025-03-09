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

@VintfStability
parcelable PresentFence {
    /**
     * The display which this commands refers to.
     */
    long display;

    /**
     * The present fence for this display.
     */
    ParcelFileDescriptor fence;

    /**
     * A LayerPresentFence is provided by the server when a LayerCommand.pictureProfileId, specified
     * by the client, results in the buffer being rendered on the display with some latency after
     * the rest of the DisplayCommand has been rendered. This can happen due to the picture
     * processing pipeline adding additional latency for the buffer, itself. LayerPresentFences are
     * intended to arrive in the same order for each buffer submission on that layer.
     *
     * Note that this violates the SurfaceControl.Transaction API contract and therefore is only
     * allowed on TV devices that require this feature to support high quality video playback on
     * large displays.
     */
    parcelable LayerPresentFence {
        /**
         * The layer which this fence refers to.
         */
        long layer;

        /**
         * The present fence for the buffer contents.
         *
         * If the buffer ends up being dropped by the server and not rendered, this fence should be
         * fired at the same time as the next buffer's present fence (or the display fence if
         * picture processing for this layer was removed).
         */
        ParcelFileDescriptor bufferFence;

        /**
         * The latency that is required for applying picture processing to the layer's buffer.
         */
        long bufferLatencyNanos;
    }
    /**
     * The LayerPresentFences for the display.
     */
    @nullable LayerPresentFence[] layerPresentFences;
}
