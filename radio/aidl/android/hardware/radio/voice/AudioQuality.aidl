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

/**
 * Audio codec which is used on GSM, UMTS, and CDMA. These values must be opaque to the Android
 * framework. Only for display.
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum AudioQuality {
    /**
     * Unspecified audio codec
     */
    UNSPECIFIED,
    /**
     * AMR (Narrowband) audio codec
     */
    AMR,
    /**
     * AMR (Wideband) audio codec
     */
    AMR_WB,
    /**
     * GSM Enhanced Full-Rate audio codec
     */
    GSM_EFR,
    /**
     * GSM Full-Rate audio codec
     */
    GSM_FR,
    /**
     * GSM Half-Rate audio codec
     */
    GSM_HR,
    /**
     * Enhanced Variable rate codec
     */
    EVRC,
    /**
     * Enhanced Variable rate codec revision B
     */
    EVRC_B,
    /**
     * Enhanced Variable rate codec (Wideband)
     */
    EVRC_WB,
    /**
     * Enhanced Variable rate codec (Narrowband)
     */
    EVRC_NW,
}
