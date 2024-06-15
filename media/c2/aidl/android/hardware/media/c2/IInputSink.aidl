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

import android.hardware.media.c2.WorkBundle;

/**
 * An `IInputSink` is a receiver of work items.
 *
 * An @ref IComponent instance can present itself as an `IInputSink` via a thin
 * wrapper.
 *
 * @sa IInputSurface, IComponent.
 */
@VintfStability
interface IInputSink {
    /**
     * Feeds work to the sink.
     *
     * @param workBundle `WorkBundle` object containing a list of `Work` objects
     *     to queue to the component.
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::BAD_INDEX` - Some component id in some `Worklet` is not valid.
     *   - `Status::CANNOT_DO` - Tunneling has not been set up for this sink, but some
     *                   `Work` object contains tunneling information.
     *   - `Status::NO_MEMORY` - Not enough memory to queue @p workBundle.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    void queue(in WorkBundle workBundle);
}
