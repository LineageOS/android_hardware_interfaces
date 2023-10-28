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

import android.hardware.common.NativeHandle;

import android.hardware.media.c2.IComponentInterface;
import android.hardware.media.c2.IConfigurable;
import android.hardware.media.c2.IGraphicBufferAllocator;
import android.hardware.media.c2.WorkBundle;
import android.os.ParcelFileDescriptor;


/**
 * Interface for an AIDL Codec2 component.
 * Components have two states: stopped and running. The running state has three
 * sub-states: executing, tripped and error.
 *
 * All methods in `IComponent` must not block. If a method call cannot be
 * completed in a timely manner, it must throw `Status::TIMED_OUT`.
 */
@VintfStability
interface IComponent {
    /**
     * The reference object from framwork to HAL C2BlockPool.
     *
     * The object will be returned when C2BlockPool is created by a framework
     * request. The object also can be destroyed using blockPoolId.
     * Using configurable framework can query/config the object in HAL(IComponent).
     */
    parcelable BlockPool {
        long blockPoolId;
        IConfigurable configurable;
    }

    /**
     * C2AIDL allocator interface along with a waitable fd.
     *
     * The interface is used from a specific type of C2BlockPool to allocate
     * graphic blocks. the waitable fd is used to create a specific type of
     * C2Fence which can be used for waiting until to allocate is not blocked.
     */
    parcelable C2AidlGbAllocator {
        IGraphicBufferAllocator igba;
        ParcelFileDescriptor waitableFd;
    }

    /**
     * Allocator for C2BlockPool.
     *
     * C2BlockPool will use a C2Allocator which is specified by an id.
     * or C2AIDL allocator interface directly.
     */
    union BlockPoolAllocator {
        int allocatorId;
        C2AidlGbAllocator allocator;
    }

    /**
     * Configures a component for a tunneled playback mode.
     *
     * A successful call to this method puts the component in the *tunneled*
     * mode. In this mode, the output `Worklet`s returned in
     * IComponentListener::onWorkDone() may not contain any buffers. The output
     * buffers are passed directly to the consumer end of a buffer queue whose
     * producer side is configured with the returned @p sidebandStream passed
     * to IGraphicBufferProducer::setSidebandStream().
     *
     * The component is initially in the non-tunneled mode by default. The
     * tunneled mode can be toggled on only before the component starts
     * processing. Once the component is put into the tunneled mode, it shall
     * stay in the tunneled mode until and only until reset() is called.
     *
     * @param avSyncHwId A resource ID for hardware sync. The generator of sync
     *     IDs must ensure that this number is unique among all services at any
     *     given time. For example, if both the audio HAL and the tuner HAL
     *     support this feature, sync IDs from the audio HAL must not clash
     *     with sync IDs from the tuner HAL.
     * @return Codec-allocated sideband stream NativeHandle. This can
     *     be passed to IGraphicBufferProducer::setSidebandStream() to
     *     establish a direct channel to the consumer.
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::OMITTED`   - The component does not support video tunneling.
     *   - `Status::BAD_STATE` - The component is already running.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    NativeHandle configureVideoTunnel(in int avSyncHwId);

    /**
     * Creates a local `C2BlockPool` backed by the given allocator and returns
     * its id.
     *
     * The returned @p blockPoolId is the only way the client can refer to a
     * `C2BlockPool` object in the component. The id can be passed to
     * setOutputSurface() or used in some C2Param objects later.
     *
     * The created `C2BlockPool` object can be destroyed by calling
     * destroyBlockPool(), reset() or release(). reset() and release() must
     * destroy all `C2BlockPool` objects that have been created.
     *
     * @param allocator AIDL allocator interface or C2Allocator specifier
     *     for C2BlockPool
     * @param out configurable Configuration interface for the created pool. This
     *     must not be null.
     * @return Created block pool information. This could be used to config/query and
     * also be used in setOutputSurface() if the allocator
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::NO_MEMORY` - Not enough memory to create the pool.
     *   - `Status::BAD_VALUE` - @p allocatorId is not recognized.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    BlockPool createBlockPool(in BlockPoolAllocator allocator);

    /**
     * Destroys a local block pool previously created by createBlockPool().
     *
     * @param blockPoolId Id of a `C2BlockPool` that was previously returned by
     *      createBlockPool().
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::NOT_FOUND` - The supplied blockPoolId is not valid.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    void destroyBlockPool(in long blockPoolId);

    /**
     * Drains the component, and optionally downstream components. This is a
     * signalling method; as such it does not wait for any work completion.
     *
     * The last `Work` item is marked as "drain-till-here", so the component is
     * notified not to wait for further `Work` before it processes what is
     * already queued. This method can also be used to set the end-of-stream
     * flag after `Work` has been queued. Client can continue to queue further
     * `Work` immediately after this method returns.
     *
     * This method must be supported in running (including tripped) states.
     *
     * `Work` that is completed must be returned via
     * IComponentListener::onWorkDone().
     *
     * @param withEos Whether to drain the component with marking end-of-stream.
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    void drain(in boolean withEos);

    /**
     * Discards and abandons any pending `Work` items for the component.
     *
     * This method must be supported in running (including tripped) states.
     *
     * `Work` that could be immediately abandoned/discarded must be returned in
     * @p flushedWorkBundle. The order in which queued `Work` items are
     * discarded can be arbitrary.
     *
     * `Work` that could not be abandoned or discarded immediately must be
     * marked to be discarded at the earliest opportunity, and must be returned
     * via IComponentListener::onWorkDone(). This must be completed within
     * 500ms.
     *
     * @return `WorkBundle` object containing flushed `Work` items.
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    WorkBundle flush();

    /**
     * Returns the @ref IComponentInterface instance associated to this
     * component.
     *
     * An @ref IConfigurable instance for the component can be obtained by calling
     * IComponentInterface::getConfigurable() on the returned @p intf.
     *
     * @return `IComponentInterface` instance. This must not be null.
     */
    IComponentInterface getInterface();

    /**
     * Queues up work for the component.
     *
     * This method must be supported in running (including tripped) states.
     *
     * It is acceptable for this method to return `OK` and return an error value
     * using the IComponentListener::onWorkDone() callback.
     *
     * @param workBundle `WorkBundle` object containing a list of `Work` objects
     *     to queue to the component.
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::BAD_INDEX` - Some component id in some `Worklet` is not valid.
     *   - `Status::CANNOT_DO` - The components are not tunneled but some `Work` object
     *                   contains tunneling information.
     *   - `Status::NO_MEMORY` - Not enough memory to queue @p workBundle.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    void queue(in WorkBundle workBundle);

    /**
     * Releases the component.
     *
     * This method must be supported in stopped state.
     *
     * This method destroys the component. Upon return, if @p status is `OK` or
     * `DUPLICATE`, all resources must have been released.
     *
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::BAD_STATE` - The component is running.
     *   - `Status::DUPLICATE` - The component is already released.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    void release();

    /**
     * Resets the component.
     *
     * This method must be supported in all (including tripped) states other
     * than released.
     *
     * This method must be supported during any other blocking call.
     *
     * This method must return within 500ms.
     *
     * When this call returns, if @p status is `OK`, all `Work` items must
     * have been abandoned, and all resources (including `C2BlockPool` objects
     * previously created by createBlockPool()) must have been released.
     *
     * If the return value is `BAD_STATE` or `DUPLICATE`, no state change is
     * expected as a response to this call. For all other return values, the
     * component must be in the stopped state.
     *
     * This brings settings back to their default, "guaranteeing" no tripped
     * state.
     *
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::BAD_STATE` - Component is in released state.
     *   - `Status::DUPLICATE` - When called during another reset call from another
     *                   thread.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    void reset();

    /**
     * Starts the component.
     *
     * This method must be supported in stopped state as well as tripped state.
     *
     * If the return value is `OK`, the component must be in the running state.
     * If the return value is `BAD_STATE` or `DUPLICATE`, no state change is
     * expected as a response to this call. Otherwise, the component must be in
     * the stopped state.
     *
     * If a component is in the tripped state and start() is called while the
     * component configuration still results in a trip, start() must succeed and
     * a new onTripped() callback must be used to communicate the configuration
     * conflict that results in the new trip.
     *
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::BAD_STATE` - Component is not in stopped or tripped state.
     *   - `Status::DUPLICATE` - When called during another start call from another
     *                   thread.
     *   - `Status::NO_MEMORY` - Not enough memory to start the component.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    void start();

    /**
     * Stops the component.
     *
     * This method must be supported in running (including tripped) state.
     *
     * This method must return within 500ms.
     *
     * Upon this call, all pending `Work` must be abandoned.
     *
     * If the return value is `BAD_STATE` or `DUPLICATE`, no state change is
     * expected as a response to this call. For all other return values, the
     * component must be in the stopped state.
     *
     * This does not alter any settings and tunings that may have resulted in a
     * tripped state.
     *
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::BAD_STATE` - Component is not in running state.
     *   - `Status::DUPLICATE` - When called during another stop call from another
     *                   thread.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    void stop();
}
