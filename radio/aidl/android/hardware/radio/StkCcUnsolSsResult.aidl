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

import android.hardware.radio.CfData;
import android.hardware.radio.RadioError;
import android.hardware.radio.SsInfoData;
import android.hardware.radio.SsRequestType;
import android.hardware.radio.SsServiceType;
import android.hardware.radio.SsTeleserviceType;
import android.hardware.radio.SuppServiceClass;

@VintfStability
parcelable StkCcUnsolSsResult {
    SsServiceType serviceType;
    SsRequestType requestType;
    SsTeleserviceType teleserviceType;
    SuppServiceClass serviceClass;
    RadioError result;
    /**
     * Valid only for all SsServiceType except SsServiceType:CF_* else empty.
     * Only one of ssInfo and cfData may contain values and the other must be empty.
     */
    SsInfoData[] ssInfo;
    /**
     * Valid for SsServiceType:CF_* else empty
     * Only one of ssInfo and cfData may contain values and the other must be empty.
     */
    CfData[] cfData;
}
