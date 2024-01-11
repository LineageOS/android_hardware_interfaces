/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.media.c2;

/**
 * Connection between an IInputSink and an IInpuSurface.
 */
@VintfStability
interface IInputSurfaceConnection {
    /**
     * Destroys the connection between an input surface and a component.
     *
     * @throws ServiceSpecificException with one of following values:
     *   - `Status::BAD_STATE` - The component is not in running state.
     *   - `Status::NOT_FOUND` - The surface is not connected to a component.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    void disconnect();

    /**
     * Signal the end of stream.

     * @throws ServiceSpecificException with one of following values:
     *   - `Status::BAD_STATE` - The component is not in running state.
     */
    void signalEndOfStream();
}
