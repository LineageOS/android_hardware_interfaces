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

import android.hardware.graphics.common.BufferUsage;
import android.hardware.graphics.common.ExtendableType;
import android.hardware.graphics.common.PixelFormat;

@VintfStability
parcelable BufferDescriptorInfo {
    /**
     * The name of the buffer in null-terminated ASCII. Useful for debugging/tracing.
     *
     * NOTE: While a well behaved client will ensure it passes a null-terminated string
     *       within the 128-byte limit, the IAllocator service implementation should be
     *       be defensive against malformed input. As such, it is recommended that
     *       IAllocator implementations proactively do `name[127] = 0` upon receiving
     *       an allocation request to enusre that the string is definitely
     *       null-terminated regardless of what the client sent.
     */
    byte[128] name;

    /**
     * The width specifies how many columns of pixels must be in the
     * allocated buffer, but does not necessarily represent the offset in
     * columns between the same column in adjacent rows. The rows may be
     * padded.
     */
    int width;

    /**
     * The height specifies how many rows of pixels must be in the
     * allocated buffer.
     */
    int height;

    /**
     * The number of image layers that must be in the allocated buffer.
     */
    int layerCount;

    /**
     * Buffer pixel format. See PixelFormat.aidl in graphics/common for
     * valid values
     */
    PixelFormat format = PixelFormat.UNSPECIFIED;

    /**
     * Buffer usage mask; valid flags can be found in the definition of
     * BufferUsage.aidl in graphics/common
     *
     * The allocator must report isSupported() == false and reject any allocations
     * with unrecognized buffer usages.
     */
    BufferUsage usage = BufferUsage.CPU_READ_NEVER;

    /**
     * The size in bytes of the reserved region associated with the buffer.
     * See getReservedRegion for more information.
     */
    long reservedSize;

    /**
     * Extensible additional options that can be set.
     *
     * This is intended for options that do not change the overall usage, but which do impact
     * how a buffer is allocated. An example of this is compression level, such as for
     * the EGL_EXT_surface_compression extension.
     *
     * The allocator must report isSupported() == false and reject any allocations
     * with unrecognized options.
     */
    ExtendableType[] additionalOptions;
}
