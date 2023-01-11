/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.tv.hdmi.cec;

import android.hardware.tv.hdmi.cec.CecLogicalAddress;

@VintfStability
parcelable CecMessage {
    /**
     * Maximum length of the message body
     */
    const int MAX_MESSAGE_BODY_LENGTH = 15;
    /**
     * logical address of the initiator
     */
    CecLogicalAddress initiator;
    /**
     * logical address of destination
     */
    CecLogicalAddress destination;
    /**
     * The maximum size of body is 15 (MAX_MESSAGE_BODY_LENGTH) as specified in
     * the section 6 of the CEC Spec 1.4b. Overflowed data must be ignored.
     */
    byte[] body;
}
