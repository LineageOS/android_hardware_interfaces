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

package android.hardware.media.c2;

/**
 * Ordering information of @ref FrameData objects. Each member is used for
 * comparing urgency: a smaller difference from a reference value indicates that
 * the associated Work object is more urgent. The reference value for each
 * member is initialized the first time it is communicated between the client
 * and the codec, and it may be updated to later values that are communicated.
 *
 * Each member of `WorkOrdinal` is stored as an unsigned integer, but the actual
 * order it represents is derived by subtracting the reference value, then
 * interpreting the result as a signed number with the same storage size (using
 * two's complement).
 *
 * @note `WorkOrdinal` is the HIDL counterpart of `C2WorkOrdinalStruct` in the
 * Codec 2.0 standard.
 */
@VintfStability
parcelable WorkOrdinal {
    /**
     * Timestamp in microseconds.
     */
    long timestampUs;
    /**
     * Frame index.
     */
    long frameIndex;
    /**
     * Component specific frame ordinal.
     */
    long customOrdinal;
}
