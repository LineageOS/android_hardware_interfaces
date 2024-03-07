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

/**
 * Mirrored Stream Classification Service (MSCS) parameters.
 * Refer to section 3.1 of the Wi-Fi QoS Management Specification v3.0.
 */
@VintfStability
parcelable MscsParams {
    /**
     * Bitmap indicating which User Priorities should be classified using MSCS.
     * The least significant bit corresponds to UP 0, and the most significant
     * bit to UP 7. Setting a bit to 1 indicates that UP should be used.
     */
    byte upBitmap;

    /**
     * Maximum user priority that can be assigned using the MSCS service.
     * Value must be between 0 and 7 (inclusive).
     */
    byte upLimit;

    /**
     * Stream timeout in μs. Must be equivalent to 60 sec or less.
     */
    int streamTimeoutUs;

    /**
     * Bitmask of available fields for a Type 4 TCLAS frame classifier.
     * See Figures 9-309 and 9-310 in the IEEE Std 802.11-2020 Standard.
     */
    @VintfStability
    @Backing(type="int")
    enum FrameClassifierFields {
        IP_VERSION = 1 << 0,
        SRC_IP_ADDR = 1 << 1,
        DST_IP_ADDR = 1 << 2,
        SRC_PORT = 1 << 3,
        DST_PORT = 1 << 4,
        DSCP = 1 << 5,
        /** Indicates Protocol if using IPv4, or Next Header if using IPv6. */
        PROTOCOL_NEXT_HDR = 1 << 6,
        /** Only applicable if using IPv6. */
        FLOW_LABEL = 1 << 7,
    }

    /**
     * Bitmask of |FrameClassifierFields| for a Type 4 TCLAS frame classifier.
     */
    byte frameClassifierMask;
}
