/*
 * Copyright (C) 2024 The Android Open Source Project
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

import android.os.ParcelFileDescriptor;

/**
 * Interface for decoder output buffer allocator for HAL process
 *
 * A graphic buffer for decoder output is allocated by the interface.
 */
@VintfStability
interface IPooledGraphicBufferAllocator {
    /**
     * A graphic buffer allocation.
     *
     * bufferId is id of buffer from a media.bufferpool2. The buffer is
     * android.hardware.HardwareBuffer.
     * fence is provided in order to signal readiness of the buffer I/O inside
     * underlying Graphics subsystem. This is called a sync fence throughout Android framework.
     */
    parcelable Allocation {
        int bufferId;
        @nullable ParcelFileDescriptor fence;
    }

    /**
     * Parameters for a graphic buffer allocation.
     *
     * Refer to AHardwareBuffer_Desc(libnativewindow) for details.
     */
    parcelable Description {
        int widthPixels;
        int heightPixels;
        int format; // AHardwareBuffer_Format
        long usage; // AHardwareBuffer_UsageFlags
    }

    /**
     * Allocate a graphic buffer.
     * deallocate() must be called after the allocated buffer is no longer needed.
     *
     * @param desc Allocation parameters.
     * @return an id of a buffer, the id is created from media.bufferpool2 in order for
     *     caching and recycling,
     *     If underlying allocator is blocked, c2::Status::Blocked will be returned.
     *     Waitable fd must be obtained using the ndk object locally. The waitable fd must
     *     be passed to the receiver during BlockPool creation request via AIDL.
     * @throws ServiceSpecificException with one of the following values:
     *   - `c2::Status::BAD_STATE` - The client is not in running states.
     *   - `c2::Status::BLOCKED`   - Underlying graphics system is blocked.
     *   - `c2::Status::CORRUPTED` - Some unknown error occurred.
     */
    Allocation allocate(in Description desc);

    /**
     * De-allocate a graphic buffer by graphic buffer's unique id.
     *
     * @param id buffer id.
     * @return {@code true} when de-allocate happened, {@code false} otherwise.
     */
    boolean deallocate(in int id);
}
