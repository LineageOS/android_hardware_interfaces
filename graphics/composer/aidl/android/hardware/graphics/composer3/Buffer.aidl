/**
 * Copyright (c) 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.graphics.composer3;

import android.hardware.common.NativeHandle;

@VintfStability
parcelable Buffer {
    /**
     * Buffer slot in the range [0, bufferSlotCount) where bufferSlotCount is
     * the parameter used when the layer was created.
     * @see IComposer.createLayer.
     * The slot is used as a buffer caching mechanism. When the Buffer.handle
     * is null, the implementation uses the previous buffer associated with this
     * slot.
     */
    int slot;

    /**
     * Buffer Handle. Can be null if this is the same buffer that was sent
     * previously on this slot.
     */
    @nullable NativeHandle handle;

    /**
     * Buffer fence that represents when it is safe to access the buffer.
     * A null fence indicates that the buffer can be accessed immediately.
     */
    @nullable ParcelFileDescriptor fence;
}
