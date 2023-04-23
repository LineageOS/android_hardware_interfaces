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

import android.hardware.HardwareBuffer;
import android.os.ParcelFileDescriptor;

/**
 * Interface for decoder output buffer allocator for HAL process
 *
 * A graphic buffer for decoder output is allocated by the interface.
 */
@VintfStability
interface IGraphicBufferAllocator {
    /**
     * A graphic buffer allocation.
     *
     * buffer is in android.hardware.HardwareBuffer.
     * fence is provided in order to signal readiness of the buffer I/O inside
     * underlying Graphics subsystem. This is called a sync fence throughout Android framework.
     */
    parcelable Allocation {
        HardwareBuffer buffer;
        ParcelFileDescriptor fence;
    }

    /**
     * Parameters for a graphic buffer allocation.
     *
     * Refer to AHardwareBuffer_Desc(libnativewindow) for details.
     */
    parcelable Description {
        int width;
        int height;
        int format;
        long usage;
    }

    /**
     * Allocate a graphic buffer.
     *
     * @param desc Allocation parameters.
     * @return an android.hardware.HardwareBuffer which is basically same to
     *     AHardwareBuffer. If underlying grpahics system is blocked, c2::Status::Blocked
     *     will be returned. In this case getWaitableFds() will return file descriptors which
     *     can be used to construct a waitable object. The waitable object will be notified
     *     when underlying graphics system is unblocked
     * @throws ServiceSpecificException with one of the following values:
     *   - `c2::Status::BAD_STATE` - The client is not in running states.
     *   - `c2::Status::BLOCKED`   - Underlying graphics system is blocked.
     *   - `c2::Status::CORRUPTED` - Some unknown error occurred.
     */
    Allocation allocate(in Description desc);

    /**
     * De-allocate a graphic buffer by graphic buffer's unique id.
     *
     * @param id graphic buffer's unique id. See also AHardwareBuffer_getId().
     * @return {@code true} when de-allocate happened, {@code false} otherwise.
     */
    boolean deallocate(in long id);

    /**
     * Fds for waitable object events.
     *
     * Fds are created by eventfd() with semaphore mode.
     * For allocEvent, An integer counter regarding dequeuable buffer count is maintained
     * by client using read()/write() to the fd. The fd can be checked whether it is
     * readable via poll(). When in readable status, the specified counter is positive
     * so allocate/dequeue can happen.
     *
     * For statusEvent, the client can notify further allocation is not feasible.
     * e.g.) life-cycle of the underlying allocator is ended.
     *
     * C2Fence object should be implemented based on this Fds. Thus, C2Fence can return
     * either by allocation being ready or allocation being infeasible by the client status
     * change.
     */
    parcelable WaitableFds {
        ParcelFileDescriptor allocEvent;
        ParcelFileDescriptor statusEvent;
    }

    /**
     * Gets waiable file descriptors.
     *
     * Use this method once and cache it in order not to create unnecessary duplicated fds.
     * The returned array will have two fds.
     *
     * If many waitable objects based on the same fd are competing, all watiable objects will be
     * notified. After being notified, they should invoke allocate(). At least one of them can
     * successfully allocate. Others not having an Allocation will have c2::Status::BLOCKED
     * as return value. They should wait again via waitable objects based on the fds which are
     * already returned from this interface.
     *
     * @return an fd array which will be wrapped to C2Fence and will be waited for
     *     until allocating is unblocked.
     */
    WaitableFds getWaitableFds();
}
