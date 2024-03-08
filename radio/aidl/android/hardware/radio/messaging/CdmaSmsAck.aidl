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
parcelable CdmaSmsAck {
    boolean errorClass;
    /**
     * SMS cause code as defined in N.S00005, 6.5.2.125.
     * Currently, only 35 (resource shortage) and 39 (other terminal problem) are reported.
     */
    int smsCauseCode;
}
