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
 * Forward Error Correction (FEC) for ATSC3.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendAtsc3Fec {
    UNDEFINED = 0,

    /**
     * hardware is able to detect and set FEC automatically
     */
    AUTO = 1 << 0,

    BCH_LDPC_16K = 1 << 1,

    BCH_LDPC_64K = 1 << 2,

    CRC_LDPC_16K = 1 << 3,

    CRC_LDPC_64K = 1 << 4,

    LDPC_16K = 1 << 5,

    LDPC_64K = 1 << 6,
}
