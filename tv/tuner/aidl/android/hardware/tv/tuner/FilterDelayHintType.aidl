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
 * Filter Delay Hint Type
 * Specifies the type of filter delay.
 * @hide
 */
@VintfStability
@Backing(type="int")
enum FilterDelayHintType {
    /**
     * Invalid type to be used as default value.
     */
    INVALID,

    /**
     * Hint that can be used by the filter implementation to make decisions about
     * delaying the filter callback until a specified amount of time has passed.
     * For time delays, the hint value contains time in milliseconds.
     */
    TIME_DELAY_IN_MS,

    /**
     * Hint that can be used by the filter implementation to make decisions about
     * delaying the filter callback until a specified amount of data has been
     * accumulated.
     * For data size delays, the hint value contains the data size in bytes.
     */
    DATA_SIZE_DELAY_IN_BYTES,
}
