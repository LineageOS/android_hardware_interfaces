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

import android.hardware.power.CompositionUpdate;
import android.hardware.power.FrameProducer;

/**
 * Object sent to PowerHAL once per frame during commit that contains relevant
 * timing data for a given set of frame producers, drawing to a given set of outputs.
 *
 * This will generally be layer objects drawing to displays, but could also represent
 * things like arbitrary graphics buffers drawing into network sockets. All frame
 * producers that participated in this composition and have associated
 * sessions will be listed under "producers".
 *
 * All timestamps use SYSTEM_TIME_MONOTONIC clock, at nanosecond resolution.
 */
@VintfStability
parcelable CompositionData {
    /**
     * Timestamp for when the message was sent, useful to combine and correlate
     * composition data with any reported info from sessions.
     *
     * The latchTime can be used alongside this for chronologically
     * ordering events that happened during latching, such as frame drop.
     */
    long timestampNanos;

    /**
     * Scheduled presentation times for each outputId, corresponding to the outputId
     * with the same index.
     */
    long[] scheduledPresentTimestampsNanos;

    /**
     * The current frame's latch time for buffers targeting its vsync, this serves
     * as the effective frame deadline unless the frame latches with GPU unsignaled.
     */
    long latchTimestampNanos;

    /**
     * The set of frame producers that tried to present this vsync on these outputs,
     * ignoring the ones without associated sessions.
     */
    FrameProducer[] producers;

    /**
     * Optional parcel containing information not bound to a specific frame,
     * such as lifecycle updates. These updates can be sent along with CompositionData
     * to minimize additional calls, when appropriate.
     */
    @nullable CompositionUpdate updateData;

    /**
     * A list of IDs corresponding to one or more outputs, such as displays,
     * that are the intended recipients of this frame composition. Each output
     * ID is guaranteed to be unique for its lifetime.
     */
    long[] outputIds;
}
