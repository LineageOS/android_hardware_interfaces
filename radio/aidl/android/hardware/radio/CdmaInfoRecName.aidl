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

package android.hardware.radio;

/**
 * Names of the CDMA info records (C.S0005 section 3.7.5)
 */
@VintfStability
@Backing(type="int")
enum CdmaInfoRecName {
    DISPLAY,
    CALLED_PARTY_NUMBER,
    CALLING_PARTY_NUMBER,
    CONNECTED_NUMBER,
    SIGNAL,
    REDIRECTING_NUMBER,
    LINE_CONTROL,
    EXTENDED_DISPLAY,
    T53_CLIR,
    T53_RELEASE,
    T53_AUDIO_CONTROL,
}
