/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.radio.ims;

/** @hide */
@VintfStability
@JavaDerive(toString=true)
@Backing(type="int")
enum ImsTrafficType {
    /** Emergency call */
    EMERGENCY,

    /** Emergency SMS */
    EMERGENCY_SMS,

    /** Voice call */
    VOICE,

    /** Video call */
    VIDEO,

    /** SMS over IMS */
    SMS,

    /** IMS registration and subscription for reg event package (signaling) */
    REGISTRATION,

    /** Ut/XCAP (XML Configuration Access Protocol) */
    UT_XCAP
}
