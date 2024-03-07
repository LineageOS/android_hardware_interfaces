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

/** @hide */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum RadioTechnology {
    UNKNOWN,
    GPRS,
    EDGE,
    UMTS,
    IS95A,
    IS95B,
    ONE_X_RTT,
    EVDO_0,
    EVDO_A,
    HSDPA,
    HSUPA,
    HSPA,
    EVDO_B,
    EHRPD,
    LTE,
    /**
     * HSPA+
     */
    HSPAP,
    /**
     * Only supports voice
     */
    GSM,
    TD_SCDMA,
    IWLAN,
    /**
     * @deprecated use LTE instead and indicate carrier aggregation through multiple
     * physical channel configurations in IRadioNetwork::currentPhysicalChannelConfigs.
     */
    LTE_CA,
    /**
     * 5G NR. This is only used in 5G Standalone mode.
     */
    NR,
}
