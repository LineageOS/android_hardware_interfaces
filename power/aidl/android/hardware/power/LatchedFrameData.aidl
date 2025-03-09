/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.power;

/**
 * Frame information for a specific frame producer on a specific composition, used to
 * provide timing information and adjust boosting or scheduling strategies for sessions
 * associated with that producer to compensate for observed behavior.
 *
 * All timestamps use SYSTEM_TIME_MONOTONIC clock, at nanosecond resolution.
 */
@VintfStability
parcelable LatchedFrameData {
    /**
     * Timestamp for the start of this frame, will be set to -1 if unknown.
     */
    long frameStartTimestampNanos;

    /**
     * Original, intended presentation time of the frame.
     *
     * This can be used along with the buffer submission timestamp to infer
     * if the frame was supposed to present during a previous composition and got delayed.
     *
     * It can be compared with the "intendedPresentTimestampNanos" on WorkDurations to
     * determine which reported duration sessions correspond with which LatchedFrameData,
     * for sessions that use both manual reporting and have associated FrameProducers
     */
    long intendedPresentTimestampNanos;

    /**
     * Timestamp of buffer submission to SF from the CPU.
     */
    long bufferSubmissionTimestampNanos;

    /**
     * Timestamp where the GPU fence signaled, will be set to -1 if the buffer
     * latched unsignaled or if the GPU was not used.
     */
    long gpuSignalTimestampNanos;

    /**
     * True if this frame used the GPU for rendering.
     */
    boolean usedGpu;

    /**
     * Optional GPU fence, sent only when the buffer latches unsignaled, and if
     * sending fences is configured on the device in SupportInfo. If the updates
     * are batched, a fence will only be sent for the most recent update.
     *
     * Can be used to get accurate gpu completion timestamps, and to boost if it
     * looks like a frame might not signal before the deadline.
     */
    @nullable ParcelFileDescriptor gpuAcquireFence;
}
