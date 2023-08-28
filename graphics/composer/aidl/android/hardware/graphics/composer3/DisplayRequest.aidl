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
parcelable DisplayRequest {
    /**
     * Instructs the client to provide a new client target buffer, even if
     * no layers are marked for client composition.
     */
    const int FLIP_CLIENT_TARGET = 1 << 0;

    /**
     * Instructs the client to write the result of client composition
     * directly into the virtual display output buffer. If any of the
     * layers are not marked as Composition.CLIENT or the given display
     * is not a virtual display, this request has no effect.
     */
    const int WRITE_CLIENT_TARGET_TO_OUTPUT = 1 << 1;

    /**
     * The display which this commands refers to.
     */
    long display;

    /**
     * The display requests for the current validated state. This must be a
     * bitwise-or of the constants in `DisplayRequest`.
     */
    int mask;

    @VintfStability
    parcelable LayerRequest {
        /**
         * The client must clear its target with transparent pixels where
         * this layer would be. The client may ignore this request if the
         * layer must be blended.
         */
        const int CLEAR_CLIENT_TARGET = 1 << 0;

        /**
         * The layer which this commands refers to.
         * @see IComposer.createLayer
         */
        long layer;
        /**
         * The layer requests for the current validated state. This must be a
         * bitwise-or of the constants in `LayerRequest`.
         */
        int mask;
    }

    /**
     * The layer requests for the current validated state.
     */
    LayerRequest[] layerRequests;
}
