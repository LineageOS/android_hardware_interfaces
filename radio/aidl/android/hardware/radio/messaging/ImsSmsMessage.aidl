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

package android.hardware.radio.messaging;

import android.hardware.radio.RadioTechnologyFamily;
import android.hardware.radio.messaging.CdmaSmsMessage;
import android.hardware.radio.messaging.GsmSmsMessage;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable ImsSmsMessage {
    RadioTechnologyFamily tech;
    /**
     * Retry if true
     */
    boolean retry;
    /**
     * Valid field if retry is set to true.
     * Contains messageRef from SendSmsResult struct corresponding to failed MO SMS.
     */
    int messageRef;
    /**
     * Valid field if tech is 3GPP2 and size = 1 else must be empty. Only one of cdmaMessage and
     * gsmMessage must be of size 1 based on the RadioTechnologyFamily and the other must be size 0.
     */
    CdmaSmsMessage[] cdmaMessage;
    /**
     * Valid field if tech is 3GPP and size = 1 else must be empty. Only one of cdmaMessage and
     * gsmMessage must be of size 1 based on the RadioTechnologyFamily and the other must be size 0.
     */
    GsmSmsMessage[] gsmMessage;
}
