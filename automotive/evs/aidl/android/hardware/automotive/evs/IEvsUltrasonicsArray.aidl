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

package android.hardware.automotive.evs;

import android.hardware.automotive.evs.IEvsUltrasonicsArrayStream;
import android.hardware.automotive.evs.UltrasonicsArrayDesc;
import android.hardware.automotive.evs.UltrasonicsDataFrameDesc;

/**
 * HAL interface for ultrasonics sensor array.
 */
@VintfStability
interface IEvsUltrasonicsArray {
    /**
     * Notifies the UltrasonicsDataDesc is consumed that was received from
     * IEvsUltrasonicsArrayStream
     *
     * @param in dataFrameDesc Ultrasonics data descriptor
     */
    void doneWithDataFrame(in UltrasonicsDataFrameDesc dataFrameDesc);

    /**
     * Returns the ultrasonic sensor array information
     *
     * @throws The description of this ultrasonic array. This must be the same
     *        value as reported by IEvsEnumerator::getUltrasonicsArrayList().
     */
    UltrasonicsArrayDesc getUltrasonicArrayInfo();

    /**
     * Specifies the depth of the buffer chain the ultrasonic sensors is
     * asked to support
     *
     * Up to this many data frames may be held concurrently by the client of IEvsUltrasonicsArray.
     * If this many frames have been delivered to the receiver without being returned
     * by doneWithFrame, the stream must skip frames until a buffer is returned for reuse.
     * It is legal for this call to come at any time, even while streams are already running,
     * in which case buffers should be added or removed from the chain as appropriate.
     * If no call is made to this entry point, the IEvsUltrasonicsArray must support at least one
     * data frame by default. More is acceptable.
     *
     * @param in bufferCount Number of buffers the client of IEvsUltrasonicsArray may hold
     *           concurrently.
     * @throws EvsResult::INVALID_ARG on invalid bufferCount.
     */
    void setMaxFramesInFlight(in int bufferCount);

    /**
     * Requests to start the stream
     *
     * @param in stream Implementation of IEvsUltrasonicsArrayStream.
     * @throws EvsResult::STREAM_ALREADY_RUNNING if stream is already running
     */
    void startStream(in IEvsUltrasonicsArrayStream stream);

    /**
     * Requests to stop the delivery of the ultrasonic array data frames
     *
     * Because delivery is asynchronous, frames may continue to arrive for
     * some time after this call returns. Each must be returned until the
     * closure of the stream is signaled to the IEvsCameraStream.
     * This function cannot fail and is ignored if the stream isn't running.
     */
    void stopStream();
}
