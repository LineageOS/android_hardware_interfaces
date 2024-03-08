/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.wifi.supplicant;

import android.hardware.wifi.supplicant.MsduDeliveryInfo;

/**
 * Additional QoS parameters as defined in Section 9.4.2.316
 * of the IEEE P802.11be/D4.0 Standard.
 */
@VintfStability
parcelable QosCharacteristics {
    /**
     * Unsigned integer specifying the minimum interval (in microseconds) between the start of
     * two consecutive service periods (SPs) that are allocated for frame exchanges.
     * The value must be non-zero.
     */
    int minServiceIntervalUs;

    /**
     * Unsigned integer specifying the maximum interval (in microseconds) between the start of two
     * consecutive SPs that are allocated for frame exchanges. The value must be non-zero.
     */
    int maxServiceIntervalUs;

    /**
     * Unsigned integer specifying the lowest data rate (in kilobits/sec)
     * for the transport of MSDUs or A-MSDUs belonging to the traffic flow.
     * The value must be non-zero.
     */
    int minDataRateKbps;

    /**
     * Unsigned integer specifying the maximum amount of time (in microseconds)
     * targeted to transport an MSDU or A-MSDU belonging to the traffic flow.
     * The value must be non-zero.
     */
    int delayBoundUs;

    /**
     * Enum values indicating which optional fields are provided.
     * See Figure 9-1001au of the IEEE P802.11be/D4.0 Standard.
     */
    @VintfStability
    @Backing(type="int")
    enum QosCharacteristicsMask {
        MAX_MSDU_SIZE = 1 << 0,
        SERVICE_START_TIME = 1 << 1,
        SERVICE_START_TIME_LINK_ID = 1 << 2,
        MEAN_DATA_RATE = 1 << 3,
        BURST_SIZE = 1 << 4,
        MSDU_LIFETIME = 1 << 5,
        MSDU_DELIVERY_INFO = 1 << 6,
    }

    /**
     * Mask of |QosCharacteristicsMask| indicating which optional fields are provided.
     */
    int optionalFieldMask;

    /**
     * Unsigned 16-bit value specifying the maximum size (in octets) of an MSDU
     * belonging to the traffic flow. The value must be non-zero if provided.
     */
    char maxMsduSizeOctets;

    /**
     * Unsigned integer specifying the anticipated time (in microseconds) when
     * the traffic starts for the associated TID.
     */
    int serviceStartTimeUs;

    /**
     * The four LSBs indicate the link identifier that corresponds to the link for which the
     * TSF timer is used to indicate the Service Start Time. The four MSBs should not be used.
     * This field is present if |serviceStartTimeUs| is included and is not present otherwise.
     */
    byte serviceStartTimeLinkId;

    /**
     * Unsigned integer indicating the data rate specified (in kilobits/sec) for transport of MSDUs
     * or A-MSDUs belonging to the traffic flow. The value must be non-zero if provided.
     */
    int meanDataRateKbps;

    /**
     * Unsigned integer specififying the maximum burst (in octets) of the MSDUs or A-MSDUs
     * belonging to the traffic flow that arrive at the MAC SAP within any time duration equal
     * to the value specified in the |delayBound| field. The value must be non-zero if provided.
     */
    int burstSizeOctets;

    /**
     * Unsigned 16-bit integer specifying the maximum amount of time (in milliseconds) since the
     * arrival of the MSDU at the MAC data service interface beyond which the MSDU is not useful
     * even if received by the receiver. The amount of time specified in this field is larger than
     * or equal to the amount of time specified in the |delayBound| field, if present. The value
     * must be non-zero if provided.
     */
    char msduLifetimeMs;

    /**
     * MSDU delivery information. See |MsduDeliveryInfo| for more details.
     */
    MsduDeliveryInfo msduDeliveryInfo;
}
