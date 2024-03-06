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

package android.hardware.radio.config;

/**
 * Multiple Enabled Profiles(MEP) mode is the jointly supported MEP mode. As per section 3.4.1.1 of
 * GSMA spec SGP.22 v3.0,there are 3 supported MEP modes: MEP-A1, MEP-A2 and MEP-B.
 * If there is no jointly supported MEP mode, supported MEP mode is set to NONE.
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum MultipleEnabledProfilesMode {
    /**
     * If there is no jointly supported MEP mode, set supported MEP mode to NONE.
     */
    NONE,
    /**
     * In case of MEP-A1, the ISD-R is selected on eSIM port 0 only and profiles are selected on
     * eSIM ports 1 and higher, with the eSIM port being assigned by the LPA or platform.
     */
    MEP_A1,
    /**
     * In case of MEP-A2, the ISD-R is selected on eSIM port 0 only and profiles are selected on
     * eSIM ports 1 and higher, with the eSIM port being assigned by the eUICC.
     */
    MEP_A2,
    /**
     * In case of MEP-B, profiles are selected on eSIM ports 0 and higher, with the ISD-R being
     * selectable on any of these eSIM ports.
     */
    MEP_B,
}
