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
 * Defining Emergency Service Category as follows:
 * - General emergency call, all categories;
 * - Police;
 * - Ambulance;
 * - Fire Brigade;
 * - Marine Guard;
 * - Mountain Rescue;
 * - Manually Initiated eCall (MIeC);
 * - Automatically Initiated eCall (AIeC);
 *
 * Category UNSPECIFIED (General emergency call, all categories) indicates that no specific
 * services are associated with this emergency number.
 *
 * Reference: 3gpp 22.101, Section 10 - Emergency Calls
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum EmergencyServiceCategory {
    /**
     * General emergency call, all categories
     */
    UNSPECIFIED = 0,
    POLICE = 1 << 0,
    AMBULANCE = 1 << 1,
    FIRE_BRIGADE = 1 << 2,
    MARINE_GUARD = 1 << 3,
    MOUNTAIN_RESCUE = 1 << 4,
    /**
     * Manually Initiated eCall (MIeC)
     */
    MIEC = 1 << 5,
    /**
     * Automatically Initiated eCall (AIeC)
     */
    AIEC = 1 << 6,
}
