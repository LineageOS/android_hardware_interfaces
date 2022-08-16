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
 * Hierarchy Type for DVBT.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FrontendDvbtHierarchy {
    UNDEFINED = 0,

    /**
     * hardware is able to detect and set Hierarchy automatically
     */
    AUTO = 1 << 0,

    HIERARCHY_NON_NATIVE = 1 << 1,

    HIERARCHY_1_NATIVE = 1 << 2,

    HIERARCHY_2_NATIVE = 1 << 3,

    HIERARCHY_4_NATIVE = 1 << 4,

    HIERARCHY_NON_INDEPTH = 1 << 5,

    HIERARCHY_1_INDEPTH = 1 << 6,

    HIERARCHY_2_INDEPTH = 1 << 7,

    HIERARCHY_4_INDEPTH = 1 << 8,
}
