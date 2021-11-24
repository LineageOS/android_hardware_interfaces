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
 * Indexes can be tagged by start point of slice groups according to ISO/IEC 14496-10.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum DemuxScAvcIndex {
    UNDEFINED = 0,

    /**
     * All blocks are coded as I blocks.
     */
    I_SLICE = 1 << 0,

    /**
     * Blocks are coded as I or P blocks.
     */
    P_SLICE = 1 << 1,

    /**
     * Blocks are coded as I, P or B blocks.
     */
    B_SLICE = 1 << 2,

    /**
     * A so-called switching I slice that is coded.
     */
    SI_SLICE = 1 << 3,

    /**
     * A so-called switching P slice that is coded.
     */
    SP_SLICE = 1 << 4,
}
