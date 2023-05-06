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

import android.hardware.radio.messaging.CdmaSmsMessage;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable CdmaSmsWriteArgs {
    const int STATUS_REC_UNREAD = 0;
    const int STATUS_REC_READ = 1;
    const int STATUS_STO_UNSENT = 2;
    const int STATUS_STO_SENT = 3;

    /**
     * Status of message. See TS 27.005 3.1
     * Values are STATUS_
     */
    int status;
    CdmaSmsMessage message;
}
