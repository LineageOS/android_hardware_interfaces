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

package android.hardware.radio.voice;

import android.hardware.radio.voice.CdmaSignalInfoRecord;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable CdmaCallWaiting {
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_UNKNOWN = 0;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_ISDN = 1;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_DATA = 3;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_TELEX = 4;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_NATIONAL = 8;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PLAN_PRIVATE = 9;

    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PRESENTATION_ALLOWED = 0;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PRESENTATION_RESTRICTED = 1;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_PRESENTATION_UNKNOWN = 2;

    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_TYPE_UNKNOWN = 0;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_TYPE_INTERNATIONAL = 1;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_TYPE_NATIONAL = 2;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_TYPE_NETWORK_SPECIFIC = 3;
    /** @deprecated Legacy CDMA is unsupported. */
    const int NUMBER_TYPE_SUBSCRIBER = 4;

    /**
     * Remote party number
     * @deprecated Legacy CDMA is unsupported.
     */
    String number;
    /**
     * Values are NUMBER_PRESENTATION_
     * @deprecated Legacy CDMA is unsupported.
     */
    int numberPresentation;
    /**
     * Remote party name
     * @deprecated Legacy CDMA is unsupported.
     */
    String name;
    /** @deprecated Legacy CDMA is unsupported. */
    CdmaSignalInfoRecord signalInfoRecord;
    /**
     * Required to support International Call Waiting
     * Values are NUMBER_TYPE_
     * @deprecated Legacy CDMA is unsupported.
     */
    int numberType;
    /**
     * Required to support International Call Waiting
     * Values are NUMBER_PLAN_
     * @deprecated Legacy CDMA is unsupported.
     */
    int numberPlan;
}
