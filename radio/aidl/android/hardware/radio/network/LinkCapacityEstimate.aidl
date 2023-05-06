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

package android.hardware.radio.network;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable LinkCapacityEstimate {
    /**
     * Estimated downlink capacity in kbps. In case of a dual connected network, this includes
     * capacity of both primary and secondary. This bandwidth estimate shall be the estimated
     * maximum sustainable link bandwidth (as would be measured at the Upper PDCP or SNDCP SAP).
     * If the DL Aggregate Maximum Bit Rate is known, this value shall not exceed the DL-AMBR for
     * the Internet PDN connection. This must be filled with 0 if network is not connected.
     */
    int downlinkCapacityKbps;
    /**
     * Estimated uplink capacity in kbps. In case of a dual connected network, this includes
     * capacity of both primary and secondary. This bandwidth estimate shall be the estimated
     * maximum sustainable link bandwidth (as would be measured at the Upper PDCP or SNDCP SAP).
     * If the UL Aggregate Maximum Bit Rate is known, this value shall not exceed the UL-AMBR for
     * the Internet PDN connection. This must be filled with 0 if network is not connected.
     */
    int uplinkCapacityKbps;
    /**
     * Estimated downlink capacity of secondary carrier in a dual connected NR mode in kbps. This
     * bandwidth estimate shall be the estimated maximum sustainable link bandwidth (as would be
     * measured at the Upper PDCP or SNDCP SAP). This is valid only in if device is connected to
     * both primary and secodary in dual connected mode. This must be filled with 0 if secondary is
     * not connected or if modem does not support this feature.
     */
    int secondaryDownlinkCapacityKbps;
    /**
     * Estimated uplink capacity secondary carrier in a dual connected NR mode in kbps. This
     * bandwidth estimate shall be the estimated maximum sustainable link bandwidth (as would be
     * measured at the Upper PDCP or SNDCP SAP). This is valid only in if device is connected to
     * both primary and secodary in dual connected mode.This must be filled with 0 if secondary is
     * not connected or if modem does not support this feature.
     */
    int secondaryUplinkCapacityKbps;
}
