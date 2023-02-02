/*
 * Copyright 2022 The Android Open Source Project
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

package android.hardware.tv.tuner;

/**
 * Audio preselection label according to ETSI EN 300 468 V1.17.1.
 * @hide
 */
@VintfStability
parcelable AudioPreselectionLabel {
    /**
     * ISO 639-2 3-character code.
     */
    String language;

    /**
     * Text information, written in the language specified, pertaining to the preselection.
     */
    String text;
}
