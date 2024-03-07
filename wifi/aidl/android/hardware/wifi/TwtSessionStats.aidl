/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.wifi;

/**
 * Target Wake Time (TWT) Session Stats
 */
@VintfStability
parcelable TwtSessionStats {
    /**
     * Average number of Tx packets in each wake duration.
     */
    int avgTxPktCount;

    /**
     * Average number of Rx packets in each wake duration.
     */
    int avgRxPktCount;

    /**
     * Average bytes per Tx packets in each wake duration.
     */
    int avgTxPktSize;

    /**
     * Average bytes per Rx packets in each wake duration.
     */
    int avgRxPktSize;

    /**
     * Average End of Service period in microseconds.
     */
    int avgEospDurationMicros;

    /**
     * Count of early terminations.
     */
    int eospCount;
}
