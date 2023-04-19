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

import android.hardware.media.c2.Buffer;
import android.hardware.media.c2.IComponent;
import android.hardware.media.c2.IComponentInterface;
import android.hardware.media.c2.IComponentListener;
import android.hardware.media.c2.IConfigurable;
import android.hardware.media.c2.StructDescriptor;

/**
 * Entry point for Codec2 HAL.
 *
 * All methods in `IComponentStore` must not block. If a method call cannot be
 * completed in a timely manner, it must throw `Status::TIMED_OUT`. The only
 * exceptions are getPoolClientManager() and getConfigurable(),  which must
 * always return immediately.
 *
 * @note This is an extension of version 1.1 of `IComponentStore`. The purpose
 * of the extension is to add support for blocking output buffer allocator.
 */
@VintfStability
interface IComponentStore {
    /**
     * Component traits.
     */
    @VintfStability
    parcelable ComponentTraits {
        @VintfStability
        @Backing(type="int")
        enum Kind {
            OTHER = 0,
            DECODER,
            ENCODER,
        }
        @VintfStability
        @Backing(type="int")
        enum Domain {
            OTHER = 0,
            VIDEO,
            AUDIO,
            IMAGE,
        }
        /**
         * Name of the component. This must be unique for each component.
         *
         * This name is use to identify the component to create in
         * createComponent() and createComponentInterface().
         */
        String name;
        /**
         * Component domain.
         */
        Domain domain;
        /**
         * Component kind.
         */
        Kind kind;
        /**
         * Rank used by `MediaCodecList` to determine component ordering. Lower
         * value means higher priority.
         */
        int rank;
        /**
         * MIME type.
         */
        String mediaType;
        /**
         * Aliases for component name for backward compatibility.
         *
         * Multiple components can have the same alias (but not the same
         * component name) as long as their media types differ.
         */
        String[] aliases;
    }

    /**
     * Copies the contents of @p src into @p dst without changing the format of
     * @p dst.
     *
     * @param src Source buffer.
     * @param dst Destination buffer.
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::CANNOT_DO` - @p src and @p dst are not compatible.
     *   - `Status::REFUSED`   - No permission to copy.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    void copyBuffer(in Buffer src, in Buffer dst);

    /**
     * Creates a component by name.
     *
     * @param name Name of the component to create. This must match one of the
     *     names returned by listComponents().
     * @param listener Callback receiver.
     * @param pool `IClientManager` object of the BufferPool in the client
     *     process. This may be null if the client does not own a BufferPool.
     * @return The created component.
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::NOT_FOUND` - There is no component with the given name.
     *   - `Status::NO_MEMORY` - Not enough memory to create the component.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     *
     * @sa IComponentListener.
     */
    IComponent createComponent(in String name, in IComponentListener listener,
        in android.hardware.media.bufferpool2.IClientManager pool);

    /**
     * Creates a component interface by name.
     *
     * @param name Name of the component interface to create. This should match
     *     one of the names returned by listComponents().
     * @return The created component interface.
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::NOT_FOUND` - There is no component interface with the given name.
     *   - `Status::NO_MEMORY` - Not enough memory to create the component interface.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    IComponentInterface createInterface(in String name);

    /**
     * Returns the @ref IConfigurable instance associated to this component
     * store.
     *
     * @return `IConfigurable` instance. This must not be null.
     */
    IConfigurable getConfigurable();

    /**
     * Returns the `IClientManager` object for the component's BufferPool.
     *
     * @return If the component store supports receiving buffers via
     *     BufferPool API, @p pool must be a valid `IClientManager` instance.
     *     Otherwise, @p pool must be null.
     */
    android.hardware.media.bufferpool2.IClientManager getPoolClientManager();

    /**
     * Returns a list of `StructDescriptor` objects for a set of requested
     * C2Param structure indices that this store is aware of.
     *
     * This operation must be performed at best effort, e.g. the component
     * store must simply ignore all struct indices that it is not aware of.
     *
     * @param indices Indices of C2Param structures to describe.
     * @return List of `StructDescriptor` objects.
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::NOT_FOUND` - Some indices were not known.
     *   - `Status::NO_MEMORY` - Not enough memory to complete this method.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    StructDescriptor[] getStructDescriptors(in int[] indices);

    /**
     * Returns the list of components supported by this component store.
     *
     * @return traits List of component traits for all components supported by
     *     this store (in no particular order).
     * @throws ServiceSpecificException with one of the following values:
     *   - `Status::NO_MEMORY` - Not enough memory to complete this method.
     *   - `Status::TIMED_OUT` - The operation cannot be finished in a timely manner.
     *   - `Status::CORRUPTED` - Some unknown error occurred.
     */
    ComponentTraits[] listComponents();
}
