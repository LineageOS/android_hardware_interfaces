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

import android.hardware.power.LatchedFrameData;

/**
 * Abstract unit of frame production. Frame production could be for outputs
 * such as layers, sets of layers, serial devices, or a network connection.
 * Frame producers are associated with one or more sessions that provide timing context
 * and thread associations for the producer.
 */
@VintfStability
parcelable FrameProducer {
    /**
     * ID of the producer, unique per-producer at a given time.
     */
    long producerId;

    /**
     * UID of the process that owns the producer.
     */
    int uid;

    /**
     * The framerate of the producer. This parameter will be set when SF is reasonably
     * confident it knows what framerate of the frame producer is, and will
     * be set to -1 in cases where SF is not sure, or it's rapidly changing.
     */
    double fps;

    /**
     * Info for the currently latching frame on this producer,
     * this value will be null if the producer tried but failed to latch.
     */
    @nullable LatchedFrameData currentlyLatchedFrame;

    /**
     * True if SF thinks a frame tried to latch for this producer but failed because the
     * CPU did not submit a buffer on time; currentlyLatchedFrame is null if this is true.
     */
    boolean cpuDeadlineMissed;

    /**
     * True if SF thinks a frame tried to latch for this producer, but failed because the
     * GPU did not finish on time; currentlyLatchedFrame is null if this is true.
     */
    boolean gpuDeadlineMissed;
}
