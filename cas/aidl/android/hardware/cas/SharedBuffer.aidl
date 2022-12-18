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

package android.hardware.cas;

import android.hardware.common.Ashmem;

/**
 * SharedBuffer describes a shared buffer which is defined by a heapBase, an
 * offset and a size. The offset is relative to the shared memory base for the
 * memory region identified by heapBase.
 * @hide
 */
@VintfStability
parcelable SharedBuffer {
    /**
     * Ashmem shared memory
     */
    Ashmem heapBase;

    /**
     * The offset from the shared memory base
     */
    long offset;

    /**
     * The size of the shared buffer in bytes
     */
    long size;
}
