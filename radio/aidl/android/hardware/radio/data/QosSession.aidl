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

import android.hardware.radio.data.Qos;
import android.hardware.radio.data.QosFilter;

/**
 * QOS session associated with a dedicated bearer
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable QosSession {
    /**
     * Unique ID of the QoS session within the data call
     */
    int qosSessionId;
    /**
     * QOS attributes
     */
    Qos qos;
    /**
     * List of QOS filters associated with this session
     */
    QosFilter[] qosFilters;
}
