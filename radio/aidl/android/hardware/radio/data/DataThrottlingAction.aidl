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

package android.hardware.radio.data;

/** @hide */
@VintfStability
@Backing(type="byte")
@JavaDerive(toString=true)
enum DataThrottlingAction {
    /*
     * Clear all existing data throttling.
     */
    NO_DATA_THROTTLING,
    /**
     * Enact secondary carrier data throttling and remove any existing data throttling on
     * anchor carrier.
     */
    THROTTLE_SECONDARY_CARRIER,
    /**
     * Enact anchor carrier data throttling and disable data on secondary carrier if currently
     * enabled.
     */
    THROTTLE_ANCHOR_CARRIER,
    /**
     * Immediately hold on to current level of throttling.
     */
    HOLD,
}
