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

import android.hardware.media.c2.IConfigurable;
import android.hardware.media.c2.IInputSink;
import android.hardware.media.c2.IInputSurfaceConnection;
import android.view.Surface;

/**
 * Input surface for a Codec2 component.
 *
 * An <em>input surface</em> is an instance of `IInputSurface`, which may be
 * created by calling IComponentStore::createInputSurface(). Once created, the
 * client may
 *   1. write data to it via the `NativeWindow` interface; and
 *   2. use it as input to a Codec2 encoder.
 *
 * @sa IInputSurfaceConnection, IComponentStore::createInputSurface(),
 *     IComponent::connectToInputSurface().
 */
@VintfStability
interface IInputSurface {
    /**
     * Returns the producer interface into the internal buffer queue.
     *
     * @return producer `Surface` instance(actually ANativeWindow). This must not
     * be null.
     */
    Surface getSurface();

    /**
     * Returns the @ref IConfigurable instance associated to this input surface.
     *
     * @return configurable `IConfigurable` instance. This must not be null.
     */
    IConfigurable getConfigurable();

    /**
     * Connects the input surface to an input sink.
     *
     * This function is generally called from inside the implementation of
     * IComponent::connectToInputSurface(), where @p sink is a thin wrapper of
     * the component that consumes buffers from this surface.
     *
     * @param sink Input sink. See `IInputSink` for more information.
     * @return connection `IInputSurfaceConnection` object. This must not be
     *     null if @p status is `OK`.
     * @throws ServiceSpecificException with one of following values:
     *   - `Status::BAD_VALUE` - @p sink is invalid.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    IInputSurfaceConnection connect(in IInputSink sink);
}
