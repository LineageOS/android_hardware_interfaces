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

import android.hardware.thermal.TemperatureType;
import android.hardware.thermal.ThrottlingSeverity;

/* @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable Temperature {
    /**
     * This temperature's type.
     */
    TemperatureType type;
    /**
     * Name of this temperature matching the TemperatureThreshold.
     * All temperatures of the same "type" must have a different "name",
     * e.g., cpu0, battery. Clients use it to match with TemperatureThreshold
     * struct.
     */
    String name;
    /**
     * For BCL, this is the current reading of the virtual sensor and the unit is
     * millivolt, milliamp, percentage for BCL_VOLTAGE, BCL_CURRENT and BCL_PERCENTAGE
     * respectively. For everything else, this is the current temperature in Celsius.
     * If not available set by HAL to NAN.
     */
    float value;
    /**
     * The current throttling level of the sensor.
     */
    ThrottlingSeverity throttlingStatus;
}
