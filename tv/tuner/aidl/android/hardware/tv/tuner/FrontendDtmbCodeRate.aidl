/*
 * Copyright 2021 The Android Open Source Project
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
 * CODERATE Type for DTMB.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendDtmbCodeRate {
    UNDEFINED = 0,

    /**
     * hardware is able to detect and set code rate automatically
     */
    AUTO = 1 << 0,

    CODERATE_2_5 = 1 << 1,

    CODERATE_3_5 = 1 << 2,

    CODERATE_4_5 = 1 << 3,
}
