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
 * @hide
 */
@VintfStability
@Backing(type="long")
enum Constant64Bit {
    /**
     * An invalid 64-bit Filter ID.
     */
    INVALID_FILTER_ID_64BIT = 0xFFFFFFFFFFFFFFFF,

    /**
     * An invalid 64-bit AV sync hardware ID.
     */
    INVALID_AV_SYNC_ID_64BIT = 0xFFFFFFFFFFFFFFFF,

    /**
     * An invalid pts.
     */
    INVALID_PRESENTATION_TIME_STAMP = 0xFFFFFFFFFFFFFFFF,
}
