/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.powerstats;

/**
 * Contains residency data for a single state
 */
@VintfStability
parcelable PowerEntityStateResidencyData {
    /**
     * Unique ID of the corresponding PowerEntityStateInfo
     */
    int powerEntityStateId;
    /**
     * Total time in milliseconds that the corresponding PowerEntity resided
     * in this state since the PowerEntity was reset
     */
    long totalTimeInStateMs;
    /**
     * Total number of times that the state was entered since the corresponding
     * PowerEntity was reset
     */
    long totalStateEntryCount;
    /**
     * Last time this state was entered. Time in milliseconds since the
     * corresponding PowerEntity was reset
     */
    long lastEntryTimestampMs;
}

