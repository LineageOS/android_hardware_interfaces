/*
 * Copyright 2022 The Android Open Source Project
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

package android.hardware.graphics.allocator;

import android.hardware.graphics.allocator.AllocationResult;
import android.hardware.graphics.allocator.BufferDescriptorInfo;

@VintfStability
interface IAllocator {
    /**
     * Allocates buffers with the properties specified by the descriptor.
     *
     * Allocations should be optimized for usage bits provided in the
     * descriptor.
     *
     * @param descriptor Properties of the buffers to allocate. This must be
     *     obtained from IMapper::createDescriptor().
     * @param count The number of buffers to allocate.
     * @return An AllocationResult containing the result of the allocation
     * @throws AllocationError on failure
     * @deprecated As of android.hardware.graphics.allocator-V2 in combination with
     *             AIMAPPER_VERSION_5 this is deprecated & replaced with allocate2.
     *             If android.hardware.graphics.mapper@4 is still in use, however, this is
     *             still required to be implemented.
     */
    AllocationResult allocate(in byte[] descriptor, in int count);

    /**
     * Allocates buffers with the properties specified by the descriptor.
     *
     * Allocations should be optimized for usage bits provided in the
     * descriptor.
     *
     * @param descriptor Properties of the buffers to allocate. This must be
     *     obtained from IMapper::createDescriptor().
     * @param count The number of buffers to allocate.
     * @return An AllocationResult containing the result of the allocation
     * @throws AllocationError on failure
     */
    AllocationResult allocate2(in BufferDescriptorInfo descriptor, in int count);

    /**
     * Test whether the given BufferDescriptorInfo is allocatable.
     *
     * If this function returns true, it means that a buffer with the given
     * description can be allocated on this implementation, unless resource
     * exhaustion occurs. If this function returns false, it means that the
     * allocation of the given description will never succeed.
     *
     * @param description the description of the buffer
     * @return supported whether the description is supported
     */
    boolean isSupported(in BufferDescriptorInfo descriptor);

    /**
     * Retrieve the library suffix to load for the IMapper SP-HAL. This library must implement the
     * IMapper stable-C interface (android/hardware/graphics/mapper/IMapper.h).
     *
     * The library that will attempt to be loaded is "/vendor/lib[64]/hw/mapper.<imapper_suffix>.so"
     */
    String getIMapperLibrarySuffix();
}
