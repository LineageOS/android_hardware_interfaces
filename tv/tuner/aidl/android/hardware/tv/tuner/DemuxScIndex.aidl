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
 * Indexes can be tagged by Start Code in PES (Packetized Elementary Stream)
 * according to ISO/IEC 13818-1.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum DemuxScIndex {
    UNDEFINED = 0,

    /**
     * Start Code is for a new I Frame
     */
    I_FRAME = 1 << 0,

    /**
     * Start Code is for a new P Frame
     */
    P_FRAME = 1 << 1,

    /**
     * Start Code is for a new B Frame
     */
    B_FRAME = 1 << 2,

    /**
     * Start Code is for a new Sequence
     */
    SEQUENCE = 1 << 3,
}
