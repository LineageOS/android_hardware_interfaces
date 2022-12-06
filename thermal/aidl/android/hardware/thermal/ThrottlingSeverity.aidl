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

package android.hardware.thermal;

/**
 * Device throttling severity
 * @hide
 */
@VintfStability
@Backing(type="int")
enum ThrottlingSeverity {
    /**
     * Not under throttling.
     */
    NONE = 0,
    /**
     * Light throttling where UX is not impacted.
     */
    LIGHT,
    /**
     * Moderate throttling where UX is not largely impacted.
     */
    MODERATE,
    /**
     * Severe throttling where UX is largely impacted.
     * Similar to 1.0 throttlingThreshold.
     */
    SEVERE,
    /**
     * Platform has done everything to reduce power.
     */
    CRITICAL,
    /**
     * Key components in platform are shutting down due to thermal condition.
     * Device functionalities will be limited.
     */
    EMERGENCY,
    /**
     * Need shutdown immediately.
     */
    SHUTDOWN,
}
