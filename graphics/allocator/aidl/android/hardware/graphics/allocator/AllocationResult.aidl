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

import android.hardware.common.NativeHandle;

/**
 * Result of an IAllocator::allocate call.
 *
 * @sa +ndk libnativewindow#AHardwareBuffer_Desc
 */
@VintfStability
parcelable AllocationResult {
    /**
     * The number of pixels between two consecutive rows of an allocated buffer, when the concept
     * of consecutive rows is defined. Otherwise, it has no meaning.
     */
    int stride;
    NativeHandle[] buffers;
}
