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
 * Audio preselection audio rendering indication according to ETSI EN 300 468 V1.17.1.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum AudioPreselectionRenderingIndicationType {
    /**
     * No preference given for the reproduction channel layout.
     */
    NOT_INDICATED,

    /**
     * Preferred reproduction channel layout is stereo.
     */
    STEREO,

    /**
     * Preferred reproduction channel layout is two-dimensional (e.g. 5.1 multi-channel).
     */
    TWO_DIMENSIONAL,

    /**
     * Preferred reproduction channel layout is three-dimensional.
     */
    THREE_DIMENSIONAL,

    /**
     * Content is pre-rendered for consumption with headphones.
     */
    HEADPHONE,
}
