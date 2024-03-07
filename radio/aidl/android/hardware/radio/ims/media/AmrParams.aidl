/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.radio.ims.media;

import android.hardware.radio.ims.media.AmrMode;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable AmrParams {
    /** mode-set: AMR codec mode to represent the bit rate */
    AmrMode amrMode;
    /**
     * octet-align: If it's set to true then all fields in the AMR/AMR-WB header
     * shall be aligned to octet boundaries by adding padding bits.
     */
    boolean octetAligned;
    /**
     * max-red: It’s the maximum duration in milliseconds that elapses between the
     * primary (first) transmission of a frame and any redundant transmission that
     * the sender will use. This parameter allows a receiver to have a bounded delay
     * when redundancy is used. Allowed values are between 0 (no redundancy will be
     * used) and 65535. If the parameter is omitted, no limitation on the use of
     * redundancy is present. See RFC 4867
     */
    int maxRedundancyMillis;
}
