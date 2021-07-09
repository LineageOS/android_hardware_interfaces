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
 * TLV Filter Type according to ITU-R BT.1869.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum DemuxTlvFilterType {
    UNDEFINED,

    /**
     * A filter to filter signaling data out from input stream, and queue the
     * data to the filter's FMQ (Fast Message Queue).
     */
    SECTION,

    /**
     * A filter to filter a TLV stream out from input stream. The output can be
     * either upper stream of another filter or queued to the filter's FMQ.
     */
    TLV,

    /**
     * A filter to strip out TLV message header and be a data source of another
     * filter.
     */
    PAYLOAD_THROUGH,
}
