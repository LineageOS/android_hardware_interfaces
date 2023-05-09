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

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable CdmaBroadcastSmsConfigInfo {
    /**
     * Defines a broadcast message identifier whose value is 0x0000 - 0xFFFF as defined in
     * C.R1001G 9.3.1 and 9.3.2.
     */
    int serviceCategory;
    /**
     * Language code of broadcast message whose value is 0x00 - 0x07 as defined in C.R1001G 9.2.
     */
    int language;
    /**
     * Selected false means message types specified in serviceCategory are not accepted,
     * while true means accepted.
     */
    boolean selected;
}
