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

package android.hardware.radio;

import android.hardware.radio.SliceServiceType;
import android.hardware.radio.SliceStatus;

/**
 * This struct represents a S-NSSAI as defined in 3GPP TS 24.501.
 */
@VintfStability
parcelable SliceInfo {
    /**
     * The type of service provided by the slice. See: 3GPP TS 24.501 Section 9.11.2.8.
     */
    SliceServiceType sst;
    /**
     * Slice differentiator is the identifier of a slice that has SliceServiceType as SST. A value
     * of -1 indicates that there is no corresponding SliceInfo of the HPLMN.
     * See: 3GPP TS 24.501 Section 9.11.2.8.
     */
    int sliceDifferentiator;
    /**
     * This SST corresponds to a SliceInfo (S-NSSAI) of the HPLMN; the SST is mapped to this value.
     * See: 3GPP TS 24.501 Section 9.11.2.8.
     */
    SliceServiceType mappedHplmnSst;
    /**
     * Present only if both sliceDifferentiator and mappedHplmnSst are also present. This SD
     * corresponds to a SliceInfo (S-NSSAI) of the HPLMN; sliceDifferentiator is mapped to this
     * value. A value of -1 indicates that there is no corresponding SliceInfo of the HPLMN.
     * See: 3GPP TS 24.501 Section 9.11.2.8.
     */
    int mappedHplmnSD;
    /**
     * Field to indicate the current status of the slice.
     */
    SliceStatus status;
}
