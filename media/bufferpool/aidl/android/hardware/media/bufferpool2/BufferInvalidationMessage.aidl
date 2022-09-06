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

/*
 * Buffer pool sends a buffer invalidation message to clients in order to
 * ensure fast reclamation of the buffers. Buffer pool implementation on
 * clients must release the invalidated buffers right away after finishing
 * the use of buffers upon receiving a buffer invalidation message.
 * Users cannot delay or control timing of the handling/reception of
 * invalidation messages. Buffer pool implementation must guarantee timely
 * handling of invalidation messages.
 */
@VintfStability
@FixedSize
parcelable BufferInvalidationMessage {
    int messageId;
    /**
     * Buffers from fromBufferId to toBufferId must be invalidated.
     * fromBufferId is inclusive, but toBufferId is not inclusive.
     * If fromBufferId > toBufferID, wrap happens. In that case
     * the wrap is based on UINT32_MAX.
     */
    int fromBufferId;
    int toBufferId;
}
