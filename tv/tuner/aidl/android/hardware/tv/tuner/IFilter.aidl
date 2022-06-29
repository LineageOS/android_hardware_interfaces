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

import android.hardware.common.NativeHandle;
import android.hardware.common.fmq.MQDescriptor;
import android.hardware.common.fmq.SynchronizedReadWrite;
import android.hardware.tv.tuner.AvStreamType;
import android.hardware.tv.tuner.DemuxFilterMonitorEventType;
import android.hardware.tv.tuner.DemuxFilterSettings;
import android.hardware.tv.tuner.FilterDelayHint;
import android.hardware.tv.tuner.IFilter;

/**
 * The Filter is used to filter wanted data according to the filter's
 * configuration.
 *
 * Note that reconfiguring Filter must happen after the Filter is stopped.
 * @hide
 */
@VintfStability
interface IFilter {
    /**
     * Get the descriptor of the filter's FMQ
     *
     * It is used by the client to get the descriptor of the filter's Fast
     * Message Queue. The data in FMQ is filtered out from demux input or upper
     * stream's filter. The data is origanized to data blocks which may have
     * different length. The length's information of one or multiple data blocks
     * is sent to client through DemuxFilterEvent. The data in each block
     * follows the stardard specified by filter's type.
     * E.X. one data block from the filter with Main_Type==TS and Sub_Type==PES
     * is Packetized Elementary Stream from Transport Stream according to
     * ISO/IEC 13818-1.
     *
     * @return the descriptor of the filter's FMQ
     */
    void getQueueDesc(out MQDescriptor<byte, SynchronizedReadWrite> queue);

    /**
     * Release the Filter instance
     *
     * It is used by the client to release the Filter instance. HAL clear
     * underneath resource. client mustn't access the instance any more.
     */
    void close();

    /**
     * Configure the filter.
     *
     * It is used by the client to configure the filter so that it can filter out
     * intended data.
     *
     * @param settings the settings of the filter.
     */
    void configure(in DemuxFilterSettings settings);

    /**
     * Configure A/V filter’s stream type. This API only applies to A/V filters.
     *
     * @param avStreamType the stream type for A/V.
     */
    void configureAvStreamType(in AvStreamType avStreamType);

    /**
     * Configure additional Context ID on the IP filter.
     *
     * @param ipCid Context Id of the IP filter.
     */
    void configureIpCid(in int ipCid);

    /**
     * Configure the monitor event.
     *
     * The event for Scrambling Status should be sent at the following two scenarios:
     *   1. When this method is called, the first detected scrambling status should be sent.
     *   2. When the Scrambling status transits into different status, event should be sent.
     *
     * The event for IP CID change should be sent at the following two scenarios:
     *   1. When this method is called, the first detected CID for the IP should be sent.
     *   2. When the CID is changed to different value for the IP filter, event should be sent.
     *
     * @param monitorEventypes the DemuxFilterMonitorEventType events to monitor. Set
     *        corresponding bit of the event to monitor. Reset to stop monitoring.
     */
    void configureMonitorEvent(in int monitorEventTypes);

    /**
     * Start the filter.
     *
     * It is used by the client to ask the filter to start filterring data.
     */
    void start();

    /**
     * Stop the filter.
     *
     * It is used by the client to ask the filter to stop filterring data.
     * It won't discard the data already filtered out by the filter. The filter
     * will be stopped and removed automatically if the demux is closed.
     */
    void stop();

    /**
     * Flush the filter.
     *
     * It is used by the client to ask the filter to flush the data which is
     * already produced but not consumed yet.
     */
    void flush();

    /**
     * Get the shared AV memory handle. Use IFilter.releaseAvHandle to release
     * the handle.
     *
     * When media filters are opened, call this API to initialize the share
     * memory handle if it's needed.
     *
     * If DemuxFilterMediaEvent.avMemory contains file descriptor, share memory
     * should be ignored.
     *
     * @param out avMemory A handle associated to the shared memory for audio or
     *         video. avMemory.data[0] is normally an fd for ION memory. When
     *         the avMemory->numFd is 0, the share memory is not initialized and
     *         does not contain valid fd. avMemory.data[avMemory.numFds] is an
     *         index used as a parameter of C2DataIdInfo to build C2 buffer in
     *         Codec. No C2 buffer would be created if the index does not exist.
     *
     * @return the size of the shared av memory. It should be ignored when the share
     *         memory is not initialized.
     */
    long getAvSharedHandle(out NativeHandle avMemory);

    /**
     * Get the 32-bit filter Id.
     *
     * It is used by the client to ask the hardware resource id for the filter.
     *
     * @return the hardware 32-bit resource Id for the filter.
     */
    int getId();

    /**
     * Get the 64-bit filter Id.
     *
     * It is used by the client to ask the hardware resource id for the filter.
     *
     * @return the hardware 64-bit resource Id for the filter.
     */
    long getId64Bit();

    /**
     * Release the handle reported by the HAL for AV memory.
     *
     * It is used by the client to notify the HAL that the AV handle won't be
     * used any more in client side, so that the HAL can mark the memory
     * presented by file descripor in the handle as released.
     *
     * @param avMemory A handle associated to the memory for audio or video.
     * @param avDataId An Id provides additional information for AV data.
     */
    void releaseAvHandle(in NativeHandle avMemory, in long avDataId);

    /**
     * Set the filter's data source.
     *
     * A filter uses demux as data source by default. If the data was packetized
     * by multiple protocols, multiple filters may need to work together to
     * extract all protocols' header. Then a filter's data source can be output
     * from another filter.
     *
     * @param filter the filter instance which provides data input. Switch to
     * use demux as data source if the filter instance is NULL.
     */
    void setDataSource(in IFilter filter);

    /**
     * Set a delay hint.
     *
     * A delay hint should be used by the filter to rate limit calls to on
     * FilterCallback.onFilterEvent by aggregating data according to the hint's
     * specification.
     */
    void setDelayHint(in FilterDelayHint hint);
}
