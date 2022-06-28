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
 * Guard Interval Type for DTMB.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendDtmbGuardInterval {
    UNDEFINED = 0,

    /**
     * hardware is able to detect and set Guard Interval automatically
     */
    AUTO = 1 << 0,

    PN_420_VARIOUS = 1 << 1,

    PN_595_CONST = 1 << 2,

    PN_945_VARIOUS = 1 << 3,

    PN_420_CONST = 1 << 4,

    PN_945_CONST = 1 << 5,

    PN_RESERVED = 1 << 6,
}
