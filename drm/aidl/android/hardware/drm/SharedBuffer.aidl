/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.drm;

import android.hardware.common.NativeHandle;

/**
 * SharedBuffer describes a decrypt buffer which is defined by a bufferId, an
 * offset and a size.  The offset is relative to the shared memory base for the
 * memory region identified by bufferId, which is established by
 * setSharedMemoryBase().
 */
@VintfStability
parcelable SharedBuffer {
    /**
     * The unique buffer identifier
     */
    int bufferId;
    /**
     * The offset from the shared memory base
     */
    long offset;
    /**
     * The size of the shared buffer in bytes
     */
    long size;
    /**
     * Handle to shared memory
     */
    NativeHandle handle;
}
