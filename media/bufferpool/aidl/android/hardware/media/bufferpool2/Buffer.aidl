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

package android.hardware.media.bufferpool2;

import android.hardware.common.NativeHandle;
import android.hardware.HardwareBuffer;

/**
 * Generic buffer for fast recycling for media/stagefright.
 *
 * During media pipeline buffer references are created, shared and
 * destroyed frequently. The underlying buffers are allocated on demand
 * by a buffer pool, and are recycled to the buffer pool when they are
 * no longer referenced by the clients.
 *
 * Initially all buffers in media HAL should be NativeHandle(actually native_handle_t).
 * HardwareBuffer(actually AHardwareBuffer) for GraphicBuffer is added from V2.
 *
 * E.g. ion or gralloc buffer
 */
@VintfStability
parcelable Buffer {
    int id;
    @nullable NativeHandle buffer;
    @nullable HardwareBuffer hwbBuffer;
}
