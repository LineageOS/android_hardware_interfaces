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

/**
 * This struct represents a S-NSSAI as defined in 3GPP TS 24.501.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable SliceInfo {
    /*
     * Not specified
     */
    const byte SERVICE_TYPE_NONE = 0;
    /*
     * Slice suitable for the handling of 5G enhanced Mobile Broadband
     */
    const byte SERVICE_TYPE_EMBB = 1;
    /**
     * Slice suitable for the handling of ultra-reliable low latency communications
     */
    const byte SERVICE_TYPE_URLLC = 2;
    /*
     * Slice suitable for the handling of massive IoT
     */
    const byte SERVICE_TYPE_MIOT = 3;

    const byte STATUS_UNKNOWN = 0;
    /**
     * Configured but not allowed or rejected yet
     */
    const byte STATUS_CONFIGURED = 1;
    /**
     * Allowed to be used
     */
    const byte STATUS_ALLOWED = 2;
    /**
     * Rejected because not available in PLMN
     */
    const byte STATUS_REJECTED_NOT_AVAILABLE_IN_PLMN = 3;
    /**
     * Rejected because not available in reg area
     */
    const byte STATUS_REJECTED_NOT_AVAILABLE_IN_REG_AREA = 4;
    /**
     * Considered valid when configured/allowed slices are not available
     */
    const byte STATUS_DEFAULT_CONFIGURED = 5;

    /**
     * The type of service provided by the slice. See: 3GPP TS 24.501 Section 9.11.2.8.
     * Values are SERVICE_TYPE_
     */
    byte sliceServiceType;
    /**
     * Slice differentiator is the identifier of a slice that has SliceServiceType as SST. A value
     * of -1 indicates that there is no corresponding SliceInfo of the HPLMN.
     * See: 3GPP TS 24.501 Section 9.11.2.8.
     */
    int sliceDifferentiator;
    /**
     * This SST corresponds to a SliceInfo (S-NSSAI) of the HPLMN; the SST is mapped to this value.
     * See: 3GPP TS 24.501 Section 9.11.2.8.
     * Values are SERVICE_TYPE_
     */
    byte mappedHplmnSst;
    /**
     * Present only if both sliceDifferentiator and mappedHplmnSst are also present. This SD
     * corresponds to a SliceInfo (S-NSSAI) of the HPLMN; sliceDifferentiator is mapped to this
     * value. A value of -1 indicates that there is no corresponding SliceInfo of the HPLMN.
     * See: 3GPP TS 24.501 Section 9.11.2.8.
     */
    int mappedHplmnSd;
    /**
     * Field to indicate the current status of the slice.
     * Values are STATUS_
     */
    byte status;
}
