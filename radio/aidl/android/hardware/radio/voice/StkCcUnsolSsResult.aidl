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

import android.hardware.radio.RadioError;
import android.hardware.radio.voice.CfData;
import android.hardware.radio.voice.SsInfoData;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable StkCcUnsolSsResult {
    const int REQUEST_TYPE_ACTIVATION = 0;
    const int REQUEST_TYPE_DEACTIVATION = 1;
    const int REQUEST_TYPE_INTERROGATION = 2;
    const int REQUEST_TYPE_REGISTRATION = 3;
    const int REQUEST_TYPE_ERASURE = 4;

    const int SERVICE_TYPE_CFU = 0;
    const int SERVICE_TYPE_CF_BUSY = 1;
    const int SERVICE_TYPE_CF_NO_REPLY = 2;
    const int SERVICE_TYPE_CF_NOT_REACHABLE = 3;
    const int SERVICE_TYPE_CF_ALL = 4;
    const int SERVICE_TYPE_CF_ALL_CONDITIONAL = 5;
    const int SERVICE_TYPE_CLIP = 6;
    const int SERVICE_TYPE_CLIR = 7;
    const int SERVICE_TYPE_COLP = 8;
    const int SERVICE_TYPE_COLR = 9;
    const int SERVICE_TYPE_WAIT = 10;
    const int SERVICE_TYPE_BAOC = 11;
    const int SERVICE_TYPE_BAOIC = 12;
    const int SERVICE_TYPE_BAOIC_EXC_HOME = 13;
    const int SERVICE_TYPE_BAIC = 14;
    const int SERVICE_TYPE_BAIC_ROAMING = 15;
    const int SERVICE_TYPE_ALL_BARRING = 16;
    const int SERVICE_TYPE_OUTGOING_BARRING = 17;
    const int SERVICE_TYPE_INCOMING_BARRING = 18;

    const int TELESERVICE_TYPE_ALL_TELE_AND_BEARER_SERVICES = 0;
    const int TELESERVICE_TYPE_ALL_TELESEVICES = 1;
    const int TELESERVICE_TYPE_TELEPHONY = 2;
    const int TELESERVICE_TYPE_ALL_DATA_TELESERVICES = 3;
    const int TELESERVICE_TYPE_SMS_SERVICES = 4;
    const int TELESERVICE_TYPE_ALL_TELESERVICES_EXCEPT_SMS = 5;

    const int SUPP_SERVICE_CLASS_NONE = 0;
    const int SUPP_SERVICE_CLASS_VOICE = 1 << 0;
    const int SUPP_SERVICE_CLASS_DATA = 1 << 1;
    const int SUPP_SERVICE_CLASS_FAX = 1 << 2;
    const int SUPP_SERVICE_CLASS_SMS = 1 << 3;
    const int SUPP_SERVICE_CLASS_DATA_SYNC = 1 << 4;
    const int SUPP_SERVICE_CLASS_DATA_ASYNC = 1 << 5;
    const int SUPP_SERVICE_CLASS_PACKET = 1 << 6;
    const int SUPP_SERVICE_CLASS_PAD = 1 << 7;
    const int SUPP_SERVICE_CLASS_MAX = 1 << 7;

    /**
     * Values are SERVICE_TYPE_
     */
    int serviceType;
    /**
     * Values are REQUEST_TYPE_
     */
    int requestType;
    /**
     * Values are TELESERVICE_TYPE_
     */
    int teleserviceType;
    /**
     * Values are a bitfield of SUPP_SERVICE_CLASS_
     */
    int serviceClass;
    RadioError result;
    /**
     * Valid only for all serviceType except SERVICE_TYPE_CF_* else empty.
     * Only one of ssInfo and cfData may contain values and the other must be empty.
     */
    SsInfoData[] ssInfo;
    /**
     * Valid for serviceType SERVICE_TYPE_CF_* else empty
     * Only one of ssInfo and cfData may contain values and the other must be empty.
     */
    CfData[] cfData;
}
