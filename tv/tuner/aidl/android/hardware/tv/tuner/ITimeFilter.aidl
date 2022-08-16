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

/**
 * Timer Filter is used by Demux to filter data based on time stamp.
 * @hide
 */
@VintfStability
interface ITimeFilter {
    /**
     * Set time stamp for time based filter.
     *
     * It is used by the client to set initial time stamp and enable time
     * filtering. The time will be incremented locally. The demux discards
     * the content which time stamp is older than the time in the time filter.
     *
     * @param timeStamp initial time stamp for the time filter. It based on
     * 90KHz has the same format as PTS (Presentation Time Stamp).
     */
    void setTimeStamp(in long timeStamp);

    /**
     * Clear the time stamp in the time filter.
     *
     * It is used by the client to clear the time value of the time filter,
     * then disable time filter.
     */
    void clearTimeStamp();

    /**
     * Get the current time in the time filter.
     *
     * It is used by the client to inquiry current time in the time filter.
     *
     * @return the current time stamp in the time filter.
     */
    long getTimeStamp();

    /**
     * Get the time from the beginning of current data source.
     *
     * It is used by the client to inquiry the time stamp from the beginning
     * of current data source.
     *
     * @return the time stamp from the beginning of current data source.
     */
    long getSourceTime();

    /**
     * Close the Time Filter instance
     *
     * It is used by the client to release the demux instance. HAL clear
     * underneath resource. client mustn't access the instance any more.
     */
    void close();
}
