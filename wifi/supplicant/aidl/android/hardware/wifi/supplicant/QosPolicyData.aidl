/*
 * Copyright (C) 2022 The Android Open Source Project
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

import android.hardware.wifi.supplicant.QosPolicyClassifierParams;
import android.hardware.wifi.supplicant.QosPolicyRequestType;

/**
 * QoS policy information in DSCP request.
 */
@VintfStability
parcelable QosPolicyData {
    /** QoS Policy identifier. */
    byte policyId;

    QosPolicyRequestType requestType;

    /**
     * DSCP value to be set for uplink traffic streams matched with
     * |classifierParams|. Applicable only when |requestType| is
     * |QOS_POLICY_ADD|.
     */
    byte dscp;

    /**
     * QoS policy classifier params. Applicable only when |requestType|
     * is |QOS_POLICY_ADD|.
     */
    QosPolicyClassifierParams classifierParams;
}
