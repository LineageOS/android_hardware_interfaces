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

import android.hardware.tv.tuner.DemuxFilterType;
import android.hardware.tv.tuner.DvrType;
import android.hardware.tv.tuner.IDvr;
import android.hardware.tv.tuner.IDvrCallback;
import android.hardware.tv.tuner.IFilter;
import android.hardware.tv.tuner.IFilterCallback;
import android.hardware.tv.tuner.ITimeFilter;

/**
 * Demultiplexer(Demux) takes a single multiplexed input and splits it into
 * one or more output.
 * @hide
 */
@VintfStability
interface IDemux {
    /**
     * Set a frontend resource as data input of the demux
     *
     * It is used by the client to specify a hardware frontend as data source of
     * this demux instance. A demux instance can have only one data source.
     */
    void setFrontendDataSource(in int frontendId);

    /**
     * Open a new filter in the demux
     *
     * It is used by the client to open a filter in the demux.
     *
     * @param type the type of the filter to be added.
     * @param bufferSize the buffer size of the filter to be opened. It's used
     * to create a FMQ(Fast Message Queue) to hold data output from the filter.
     * @param cb the callback for the filter to be used to send notifications
     * back to the client.
     *
     * @return the filter instance of the newly added.
     */
    IFilter openFilter(in DemuxFilterType type, in int bufferSize,
        in IFilterCallback cb);

    /**
     * Open time filter of the demux
     *
     * It is used by the client to open time filter of the demux.
     *
     * @return the time filter instance of the newly added.
     */
    ITimeFilter openTimeFilter();

    /**
     * Get hardware sync ID for audio and video.
     *
     * It is used by the client to get the hardware sync ID for audio and video.
     *
     * @param filter the filter instance.
     *
     * @return the ID of hardware A/V sync.
     */
    int getAvSyncHwId(in IFilter filter);

    /**
     * Get current time stamp to use for A/V sync
     *
     * It is used by the client to get current time stamp for A/V sync. HW is
     * supported to increment and maintain current time stamp.
     *
     * @param avSyncHwId the hardware id of A/V sync.
     *
     * @return the current time stamp of hardware A/V sync. The time stamp
     * based on 90KHz has the same format as PTS (Presentation Time Stamp).
     */
    long getAvSyncTime(in int avSyncHwId);

    /**
     * Close the Demux instance
     *
     * It is used by the client to release the demux instance. HAL clear
     * underneath resource. client mustn't access the instance any more.
     */
    void close();

    /**
     * Open a DVR (Digital Video Record) instance in the demux
     *
     * It is used by the client to record and playback.
     *
     * @param type specify which kind of DVR to open.
     * @param bufferSize the buffer size of the output to be added. It's used to
     * create a FMQ(Fast Message Queue) to hold data from selected filters.
     * @param cb the callback for the DVR to be used to send notifications
     * back to the client.
     *
     * @return the newly opened DVR instance.
     */
    IDvr openDvr(in DvrType type, in int bufferSize, in IDvrCallback cb);

    /**
     * Connect Conditional Access Modules (CAM) through Common Interface (CI)
     *
     * It is used by the client to connect CI-CAM. The demux uses the output
     * from the frontend as the input by default, and must change to use the
     * output from CI-CAM as the input after this call take place.
     *
     * @param ciCamId specify CI-CAM Id to connect.
     */
    void connectCiCam(in int ciCamId);

    /**
     * Disconnect Conditional Access Modules (CAM)
     *
     * It is used by the client to disconnect CI-CAM. The demux will use the
     * output from the frontend as the input after this call take place.
     *
     */
    void disconnectCiCam();
}
