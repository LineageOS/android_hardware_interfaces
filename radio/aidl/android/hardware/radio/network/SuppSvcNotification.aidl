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

package android.hardware.radio.network;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable SuppSvcNotification {
    /**
     * Notification type
     * false = MO intermediate result code
     * true = MT unsolicited result code
     */
    boolean isMT;
    /**
     * Result code. See 27.007 7.17.
     */
    int code;
    /**
     * CUG index. See 27.007 7.17.
     */
    int index;
    /**
     * "type" from 27.007 7.17 (MT only).
     */
    int type;
    /**
     * "number" from 27.007 7.17. MT only, may be empty string.
     */
    String number;
}
