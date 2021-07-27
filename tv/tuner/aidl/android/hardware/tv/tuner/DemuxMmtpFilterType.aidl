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
 * MMTP Filter Type according to ISO/IEC 23008-1
 * @hide
 */
@VintfStability
@Backing(type="int")
enum DemuxMmtpFilterType {
    UNDEFINED,

    /**
     * A filter to filter signaling data out from input stream, and queue the
     * data to the filter's FMQ (Fast Message Queue).
     */
    SECTION,

    /**
     * A filter to filter MFU (Media fragment unit) out from input stream, and
     * queue the data to the filter's FMQ.
     */
    PES,

    /**
     * A filter to filter a MMTP stream out from input stream, and queue the
     * data to the filter's FMQ.
     */
    MMTP,

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
     * A filter to filter data out from input stream, and queue the data to the
     * buffer of the record.
     */
    RECORD,

    /**
     * A filter to filter application data out from input stream, and queue the
     * data to the filter's FMQ.
     */
    DOWNLOAD,
}
