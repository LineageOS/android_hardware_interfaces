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
 * Indexes can be tagged through TS (Transport Stream) header.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum DemuxTsIndex {
    FIRST_PACKET = 1 << 0,

    PAYLOAD_UNIT_START_INDICATOR = 1 << 1,

    CHANGE_TO_NOT_SCRAMBLED = 1 << 2,

    CHANGE_TO_EVEN_SCRAMBLED = 1 << 3,

    CHANGE_TO_ODD_SCRAMBLED = 1 << 4,

    DISCONTINUITY_INDICATOR = 1 << 5,

    RANDOM_ACCESS_INDICATOR = 1 << 6,

    PRIORITY_INDICATOR = 1 << 7,

    PCR_FLAG = 1 << 8,

    OPCR_FLAG = 1 << 9,

    SPLICING_POINT_FLAG = 1 << 10,

    PRIVATE_DATA = 1 << 11,

    ADAPTATION_EXTENSION_FLAG = 1 << 12,

    /**
     * Index the address of MMT Packet Table(MPT).
     */
    MPT_INDEX_MPT = 1 << 16,

    /**
     * Index the address of Video.
     */
    MPT_INDEX_VIDEO = 1 << 17,

    /**
     * Index the address of Audio.
     */
    MPT_INDEX_AUDIO = 1 << 18,

    /**
     * Index to indicate this is a target of timestamp extraction for video.
     */
    MPT_INDEX_TIMESTAMP_TARGET_VIDEO = 1 << 19,

    /**
     * Index to indicate this is a target of timestamp extraction for audio.
     */
    MPT_INDEX_TIMESTAMP_TARGET_AUDIO = 1 << 20,
}
