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
 * A status of data in the filter's buffer.
 * @hide
 */
@VintfStability
@Backing(type="byte")
enum DemuxFilterStatus {
    /**
     * The data in the filter buffer is ready to be read. It can also be used to know the STC
     * (System Time Clock) ready status if it's PCR filter.
     */
    DATA_READY = 1 << 0,

    /**
     * The available data amount in the filter buffer is at low level which is
     * set to 25 percent by default.
     */
    LOW_WATER = 1 << 1,

    /**
     * The available data amount in the filter buffer is at high level which is
     * set to 75 percent by default.
     */
    HIGH_WATER = 1 << 2,

    /**
     * The data in the filter buffer is full and newly filtered data is being
     * discarded.
     */
    OVERFLOW = 1 << 3,

    /**
     * Indicating there is no data coming to the filter.
     */
    NO_DATA = 1 << 4,
}
