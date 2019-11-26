/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.hardware.automotive.occupant_awareness;

@VintfStability
@Backing(type="byte")
enum ConfidenceLevel {
    /**
     * No prediction could be made.
     */
    NONE,
    /**
     * Best-guess, low-confidence prediction. Predictions exceeding this threshold are adequate
     * for non-critical applications.
     */
    LOW,
    /**
     * High-confidence prediction. Predictions exceeding this threshold are adequate for
     * applications that require reliable predictions.
     */
    HIGH,
    /**
     * Highest confidence rate achievable.
     */
    MAX,
}
