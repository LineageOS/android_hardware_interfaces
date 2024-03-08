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

package android.hardware.health;

import android.hardware.health.BatteryPartStatus;

/*
 * Battery health data
 */
@VintfStability
parcelable BatteryHealthData {
    /**
     * Battery manufacturing date is reported in epoch.
     */
    long batteryManufacturingDateSeconds;
    /**
     * The date of first usage is reported in epoch.
     */
    long batteryFirstUsageSeconds;
    /**
     * Measured battery state of health (remaining estimate full charge capacity
     * relative to the rated capacity in %).
     * Value must be 0 if batteryStatus is UNKNOWN.
     * Otherwise, value must be in the range 0 to 100.
     */
    long batteryStateOfHealth;
    /**
     * Serial number of the battery. Null if not supported. If supported, a string of at least 6
     * alphanumeric characters. Characters may either be upper or lower case, but for comparison
     * and uniqueness purposes, must be treated as case-insensitive.
     */
    @nullable String batterySerialNumber;
    /**
     * Indicator for part originality of the battery.
     */
    BatteryPartStatus batteryPartStatus = BatteryPartStatus.UNSUPPORTED;
}
