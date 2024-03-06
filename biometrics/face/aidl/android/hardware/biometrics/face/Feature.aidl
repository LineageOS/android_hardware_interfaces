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

/**
 * @hide
 */
@VintfStability
@Backing(type="byte")
enum Feature {
    /**
     * Require the user to look at the device during enrollment and authentication. This feature
     * can be disabled to accommodate people who have limited vision.
     */
    REQUIRE_ATTENTION,

    /**
     * Require a diverse set of poses during enrollment. This feature can be disabled to accommodate
     * people with limited mobility.
     */
    REQUIRE_DIVERSE_POSES,

    /**
     * Enable debugging functionality.
     */
    DEBUG,
}
