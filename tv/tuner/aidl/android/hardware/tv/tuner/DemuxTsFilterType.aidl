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
 * TS Filter Type according to ISO/IEC 13818-1
 * @hide
 */
@VintfStability
@Backing(type="int")
enum DemuxTsFilterType {
    UNDEFINED,

    /**
     * A filter to filter Section data out from input stream, and queue the
     * data to the filter's FMQ (Fast Message Queue).
     */
    SECTION,

    /**
     * A filter to filter Packetized Elementary Stream data out from input
     * stream, and queue the data to the filter's FMQ.
     */
    PES,

    /**
     * A filter to filter a Transport Stream out from input stream, and queue
     * the data to the filter's FMQ.
     */
    TS,

    /**
     * A filter to filter Audio data out from input stream, and send Audio's
     * Metadata to client through onFilterEvent.
     */
    AUDIO,

    /**
     * A filter to filter Video data out from input stream, and send Video's
     * Metadata to client through onFilterEvent.
     */
    VIDEO,

    /**
     * A filter to set PCR (Program Clock Reference) channel from input stream.
     */
    PCR,

    /**
     * A filter to filter data out from input stream, and queue the data to the
     * buffer of the record.
     */
    RECORD,

    /**
     * A filter to filter out Timed External Media Information (TEMI) according
     * to ISO/IEC 13818-1:2013/ DAM 6 from input stream, and send TEMI event to
     * client through onFilterEvent.
     */
    TEMI,
}
