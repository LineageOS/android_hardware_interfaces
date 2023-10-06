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

import android.hardware.tv.tuner.AudioPreselection;

/**
 * Audio presentation metadata.
 * @hide
 */
@VintfStability
parcelable AudioPresentation {
    /**
     * Audio preselection.
     */
    AudioPreselection preselection;

    /**
     * Dolby AC-4 short program id specified in ETSI TS 103 190-2. For use in conjunction with
     * an audio preselection to ensure contininuity of personalized experience during program
     * transitions.
     */
    int ac4ShortProgramId = -1;
}
