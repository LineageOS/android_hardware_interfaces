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
 * Start Code Index type to be used in the filter for record
 * @hide
 */
@VintfStability
@Backing(type="int")
enum DemuxRecordScIndexType {
    /**
     * Don't use SC index
     */
    NONE,

    /**
     * Use Start Code index
     */
    SC,

    /**
     * Use Start Code index for HEVC
     */
    SC_HEVC,

    /**
     * Use Start Code index for AVC
     */
    SC_AVC,

    /**
     * Use Start Code index for VVC
     */
    SC_VVC,
}
