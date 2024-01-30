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

import android.hardware.radio.data.OsAppId;

/**
 * This struct represents a traffic descriptor. A valid struct must have at least one of the
 * optional values present. This is based on the definition of traffic descriptor in
 * TS 24.526 Section 5.2.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable TrafficDescriptor {
    /**
     * DNN stands for Data Network Name and represents an APN as defined in 3GPP TS 23.003.
     */
    @nullable String dnn;
    /**
     * Indicates the OsId + OsAppId (used as category in Android).
     */
    @nullable OsAppId osAppId;
}
