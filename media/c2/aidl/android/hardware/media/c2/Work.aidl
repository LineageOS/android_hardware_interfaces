/*
 * Copyright (C) 2022 The Android Open Source Project
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

import android.hardware.media.c2.FrameData;
import android.hardware.media.c2.Status;
import android.hardware.media.c2.Worklet;

/**
 * A collection of input data to and output data from the component.
 *
 * A `Work` object holds information about a single work item. It is created by
 * the client and passed to the component via IComponent::queue(). The component
 * has two ways of returning a `Work` object to the client:
 *   1. If the queued `Work` object has been successfully processed,
 *      IComponentListener::onWorkDone() shall be called to notify the listener,
 *      and the output shall be included in the returned `Work` object.
 *   2. If the client calls IComponent::flush(), a `Work` object that has not
 *      been processed shall be returned.
 *
 * `Work` is a part of @ref WorkBundle.
 */
@VintfStability
parcelable Work {
    /**
     * Additional work chain info not part of this work.
     */
    byte[] chainInfo;
    /**
     * @ref FrameData for the input.
     */
    FrameData input;
    /**
     * The chain of `Worklet`s.
     *
     * The length of #worklets is 1 when tunneling is not enabled.
     *
     * If #worklets has more than a single element, the tunnels between
     * successive components of the work chain must have been successfully
     * pre-registered at the time that the `Work` is submitted. Allocating the
     * output buffers in the `Worklet`s is the responsibility of each component
     * in the chain.
     *
     * Upon `Work` submission, #worklets must be an appropriately sized vector
     * containing `Worklet`s with @ref Worklet.hasOutput set to `false`. After a
     * successful processing, all but the final `Worklet` in the returned
     * #worklets must have @ref Worklet.hasOutput set to `false`.
     */
    Worklet[] worklets;
    /**
     * The number of `Worklet`s successfully processed in this chain.
     *
     * This must be initialized to 0 by the client when the `Work` is submitted,
     * and it must contain the number of `Worklet`s that were successfully
     * processed when the `Work` is returned to the client.
     *
     * #workletsProcessed cannot exceed the length of #worklets. If
     * #workletsProcessed is smaller than the length of #worklets, #result
     * cannot be `OK`.
     */
    int workletsProcessed;
    /**
     * The final outcome of the `Work` (corresponding to #workletsProcessed).
     *
     * The value of @ref Status.OK implies that all `Worklet`s have been
     * successfully processed.
     */
    Status result;
}
