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
     * @return An AllocationResult containing the result of an error, or
     *         an AllocationError status
     */
    AllocationResult allocate(in byte[] descriptor, in int count);
}
