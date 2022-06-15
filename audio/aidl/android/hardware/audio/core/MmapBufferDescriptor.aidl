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

package android.hardware.audio.core;

import android.hardware.common.Ashmem;

/**
 * MMap buffer descriptor is used by streams opened in MMap No IRQ mode.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable MmapBufferDescriptor {
    /**
     * MMap memory buffer.
     */
    Ashmem sharedMemory;
    /**
     * Transfer size granularity in frames.
     */
    long burstSizeFrames;
    /**
     * Attributes describing the buffer. Bitmask indexed by FLAG_INDEX_*
     * constants.
     */
    int flags;

    /**
     * Whether the buffer can be securely shared to untrusted applications
     * through the AAudio exclusive mode.
     *
     * Only set this flag if applications are restricted from accessing the
     * memory surrounding the audio data buffer by a kernel mechanism.
     * See Linux kernel's dma-buf
     * (https://www.kernel.org/doc/html/v4.16/driver-api/dma-buf.html).
     */
    const int FLAG_INDEX_APPLICATION_SHAREABLE = 0;
}
