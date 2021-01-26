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

package android.hardware.biometrics.face;

@VintfStability
@Backing(type="byte")
enum Feature {
    /**
     * Do not require the user to look at the device during enrollment and authentication. Note
     * this is to accommodate people who have limited vision.
     */
    WAVE_ATTENTION_REQUIREMENT,

    /**
     * Do not require a diverse set of poses during enrollment. This is to accommodate people with
     * limited mobility.
     */
    WAVE_DIVERSE_POSES_REQUIREMENT,

    /**
     * Enable debugging functionality.
     */
    DEBUG,
}

