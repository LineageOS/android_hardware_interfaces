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

/* @hide */
@VintfStability
@JavaDerive(toString=true)
parcelable TemperatureThreshold {
    /**
     * This temperature's type.
     */
    TemperatureType type;
    /**
     * Name of this temperature matching the Temperature struct.
     * All temperatures of the same "type" must have a different "name",
     * e.g., cpu0, battery. Clients use it to match Temperature struct.
     */
    String name;
    /**
     * Hot throttling temperature constant for this temperature sensor in
     * level defined in ThrottlingSeverity including shutdown. Throttling
     * happens when temperature >= threshold. If not available, set to NAN.
     * Unit is same as Temperature's value.
     * The array size must be the same as ThrottlingSeverity's enum cardinality.
     */
    float[] hotThrottlingThresholds;
    /**
     * Cold throttling temperature constant for this temperature sensor in
     * level defined in ThrottlingSeverity including shutdown. Throttling
     * happens when temperature <= threshold. If not available, set to NAN.
     * Unit is same as Temperature's value.
     * The array size must be the same as ThrottlingSeverity's enum cardinality.
     */
    float[] coldThrottlingThresholds;
}
