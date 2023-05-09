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

import android.hardware.radio.RadioTechnology;

/** @hide */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum RadioAccessFamily {
    UNKNOWN = 1 << RadioTechnology.UNKNOWN,
    GPRS = 1 << RadioTechnology.GPRS,
    EDGE = 1 << RadioTechnology.EDGE,
    UMTS = 1 << RadioTechnology.UMTS,
    IS95A = 1 << RadioTechnology.IS95A,
    IS95B = 1 << RadioTechnology.IS95B,
    ONE_X_RTT = 1 << RadioTechnology.ONE_X_RTT,
    EVDO_0 = 1 << RadioTechnology.EVDO_0,
    EVDO_A = 1 << RadioTechnology.EVDO_A,
    HSDPA = 1 << RadioTechnology.HSDPA,
    HSUPA = 1 << RadioTechnology.HSUPA,
    HSPA = 1 << RadioTechnology.HSPA,
    EVDO_B = 1 << RadioTechnology.EVDO_B,
    EHRPD = 1 << RadioTechnology.EHRPD,
    LTE = 1 << RadioTechnology.LTE,
    HSPAP = 1 << RadioTechnology.HSPAP,
    GSM = 1 << RadioTechnology.GSM,
    TD_SCDMA = 1 << RadioTechnology.TD_SCDMA,
    IWLAN = 1 << RadioTechnology.IWLAN,
    /** @deprecated use LTE instead. */
    LTE_CA = 1 << RadioTechnology.LTE_CA,
    /**
     * 5G NR. This is only use in 5G Standalone mode.
     */
    NR = 1 << RadioTechnology.NR,
}
