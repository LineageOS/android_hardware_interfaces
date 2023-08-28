/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;

import android.hardware.common.fmq.MQDescriptor;
import android.hardware.common.fmq.SynchronizedReadWrite;
import android.hardware.tv.tuner.DvrSettings;
import android.hardware.tv.tuner.IFilter;

/**
 * Digtal Video Record (DVR) interface provides record control on Demux's
 * output buffer and playback control on Demux's input buffer.
 * @hide
 */
@VintfStability
interface IDvr {
    /**
     * Get the descriptor of the DVR's FMQ
     *
     * It is used by the client to get the descriptor of the DVR's Fast
     * Message Queue. The FMQ is used to transfer record or playback data
     * between the client and the HAL.
     *
     * @return the descriptor of the DVR's FMQ
     */
    void getQueueDesc(out MQDescriptor<byte, SynchronizedReadWrite> queue);

    /**
     * Configure the DVR.
     *
     * It is used by the client to configure the DVR interface.
     *
     * @param settings the settings of the DVR interface.
     */
    void configure(in DvrSettings settings);

    /**
     * Attach one filter to DVR interface for recording.
     *
     * It is used by the client to add the data filtered out from the filter
     * to record.
     *
     * @param filter the instance of the attached filter.
     */
    void attachFilter(in IFilter filter);

    /**
     * Detach one filter from the DVR's recording.
     *
     * It is used by the client to remove the data of the filter from DVR's
     * recording.
     *
     * @param filter the instance of the detached filter.
     */
    void detachFilter(in IFilter filter);

    /**
     * Start DVR.
     *
     * It is used by the client to ask the DVR to start consuming playback data
     * or producing data for record.
     */
    void start();

    /**
     * Stop DVR.
     *
     * It is used by the client to ask the DVR to stop consuming playback data
     * or producing data for record.
     */
    void stop();

    /**
     * Flush DVR data.
     *
     * It is used by the client to ask the DVR to flush the data which is
     * not consumed by HAL for playback or the client for record yet.
     */
    void flush();

    /**
     * close the DVR instance to release resource for DVR.
     *
     * It is used by the client to close the DVR instance, and HAL clears
     * underneath resource for this DVR instance. Client mustn't access the
     * instance any more and all methods should return a failure.
     */
    void close();

    /**
     * Set status check time interval.
     *
     * This time interval hint will be used by the Dvr to decide how often
     * to evaluate data.
     */
    void setStatusCheckIntervalHint(in long milliseconds);
}
