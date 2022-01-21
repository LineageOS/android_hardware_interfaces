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

import android.hardware.common.Ashmem;

/**
 * Structure that describes the data frame received from an ultrasonics array.
 *
 * Each data frame returned consists of received waveform signals from a subset
 * of sensors in an array as indicated by the receiversIdList. The signal is
 * transmitted at a particular time instant indicated by timestampNs from a
 * subset of sensors in the array as provided in the transmittersIdList.
 */
@VintfStability
parcelable UltrasonicsDataFrameDesc {
    /**
     * Timestamp of the start of the transmit signal for this data frame.
     * Timestamp unit is nanoseconds and is obtained from android::elapsedRealtimeNanos().
     * timeOfFlight readings are future-deltas to this timestamp.
     */
    long timestampNs;
    /**
     * Identifier of data frame. Used by implementation for managing multiple frames in flight.
     */
    int id;
    /**
     * List of indexes of sensors in range [0, sensorCount - 1] that
     * transmitted the signal for this data frame.
     */
    byte[] transmittersIdList;
    /**
     * List of indexes of sensors in range [0, sensorCount - 1] that received
     * the signal. The order of ids must match the order of the waveforms in the
     * waveformsData.
     * Size of list is upper bound by maxReceiversCount.
     */
    byte[] receiversIdList;
    /**
     * List of the number of readings corresponding to each ultrasonics sensor in
     * the receiversIdList. Order of the readings count must match the order in
     * receiversIdList.
     * Size of list is upper bound by maxReadingsPerSensorCount.
     */
    int[] receiversReadingsCountList;
    /**
     * Shared memory object containing the waveforms data. Contains one waveform
     * for each sensor specified in receiversIdList, in order.
     * Each waveform is represented by a number of readings, which are sample
     * points on the waveform. The number of readings for each waveform is as
     * specified in the receiversReadingsCountList.
     * Each reading is a pair of time Of flight and resonance.
     * Time of flight (float): Time between transmit and receive signal in nanoseconds.
     * Resonance (float): Resonance at time on waveform in range [0.0, 1.0].
     *
     * The structure of shared memory (example with 2 waveforms, each with 2 readings):
     *
     * Byte: |   0    |  1-4  |  5-8  | 9-12  | 13-16 ||   17   |  18-21 | 22-25  | 26-29 | 30-33 |
     * Data: | RecId1 | TOF1  | RES1  | TOF2  | RES2  || RecId2 |  TOF1  |  RES1  | TOF2  | RES2  |
     *       |              Waveform1                 ||             Waveform2                    |
     * Here:
     * RecId : Receiver's Id. Order matches the receiversIdList, type uint8_t
     * TOF : Time of flight, type float (4 bytes)
     * RES : Resonance, type float (4 bytes)
     * Note: All readings and waveforms are contigious with no padding.
     */
    Ashmem waveformsData;
}
