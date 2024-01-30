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

package android.hardware.radio.sim;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable SelectUiccSub {
    const int SUBSCRIPTION_TYPE_1 = 0;
    const int SUBSCRIPTION_TYPE_2 = 1;
    const int SUBSCRIPTION_TYPE_3 = 2;

    const int ACT_STATUS_DEACTIVATE = 0;
    const int ACT_STATUS_ACTIVATE = 1;

    int slot;
    /**
     * Array subscriptor from applications[RadioConst:CARD_MAX_APPS] in getIccCardStatus()
     */
    int appIndex;
    /**
     * Values are SUBSCRIPTION_TYPE_
     */
    int subType;
    /**
     * Values are ACT_STATUS_
     */
    int actStatus;
}
