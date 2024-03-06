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

package android.hardware.radio.data;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable PcoDataInfo {
    /**
     * Context ID, uniquely identifies this call
     */
    int cid;
    /**
     * One of the PDP_type values in TS 27.007 section 10.1.1. For example, "IP", "IPV6", "IPV4V6"
     */
    String bearerProto;
    /**
     * The protocol ID for this box. Note that only IDs from FF00H - FFFFH are accepted.
     * If more than one is included from the network, multiple calls must be made to send
     * all of them.
     */
    int pcoId;
    /**
     * Carrier-defined content. It is binary, opaque and loosely defined in LTE Layer 3 spec 24.008
     */
    byte[] contents;
}
