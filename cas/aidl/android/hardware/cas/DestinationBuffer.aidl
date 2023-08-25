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

import android.hardware.cas.SharedBuffer;
import android.hardware.common.NativeHandle;

/**
 * @hide
 */
@VintfStability
union DestinationBuffer {
    /**
     * If the buffer is SharedBuffer, the descrambled data must be written
     * to user-space non-secure shared memory.
     */
    SharedBuffer nonsecureMemory;

    /**
     * If the buffer is presented by NativeHandle, the descrambled data must be written
     * to secure memory referenced by the vendor's buffer allocator.
     */
    NativeHandle secureMemory;
}
