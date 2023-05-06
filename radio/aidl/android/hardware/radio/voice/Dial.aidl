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

import android.hardware.radio.voice.UusInfo;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable Dial {
    /**
     * Use subscription default value
     */
    const int CLIR_DEFAULT = 0;
    /**
     * Restrict CLI presentation
     */
    const int CLIR_INVOCATION = 1;
    /**
     * Allow CLI presentation
     */
    const int CLIR_SUPPRESSION = 2;

    String address;
    /**
     * Values are CLIR_
     */
    int clir;
    /**
     * Vector of User-User Signaling Information
     */
    UusInfo[] uusInfo;
}
