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
 * Filter Main Type specifies the protocol that the filter use to extract data
 * from input stream.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum DemuxFilterMainType {
    UNDEFINED = 0,

    /**
     * Transport Stream according to ISO/IEC 13818-1.
     */
    TS = 1 << 0,

    /**
     * MPEG Media Transport Protocol according to ISO/IEC 23008-1.
     */
    MMTP = 1 << 1,

    /**
     * Internet Protocol.
     */
    IP = 1 << 2,

    /**
     * Type Length Value according to ITU-R BT.1869.
     */
    TLV = 1 << 3,

    /**
     * ATSC Link-Layer Protocol according to A/330 ATSC3.0.
     */
    ALP = 1 << 4,
}
