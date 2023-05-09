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
 * Display Info Rec as defined in C.S0005 section 3.7.5.1. Extended Display Info Rec as defined in
 * C.S0005 section 3.7.5.16. Note that the Extended Display info rec contains multiple records of
 * the form: display_tag, display_len, and display_len occurrences of the char field if the
 * display_tag is not 10000000 or 10000001. To save space, the records are stored consecutively in
 * a byte buffer. The display_tag, display_len and chari fields are all 1 byte.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable CdmaDisplayInfoRecord {
    const int CDMA_ALPHA_INFO_BUFFER_LENGTH = 64;
    /**
     * Max length = CDMA_ALPHA_INFO_BUFFER_LENGTH
     */
    String alphaBuf;
}
