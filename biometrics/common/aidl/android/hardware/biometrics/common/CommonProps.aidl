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

package android.hardware.biometrics.common;

import android.hardware.biometrics.common.ComponentInfo;
import android.hardware.biometrics.common.SensorStrength;

@VintfStability
parcelable CommonProps {
    /**
     * A statically configured unique ID that identifies a single biometric sensor. IDs must start
     * at zero and increment by one for each unique sensor. Note that ID allocations are shared
     * between all biometric modalities (e.g. fingerprint, face, iris), and a single ID must never
     * be claimed by more than a single sensor.
     */
    int sensorId;

    /**
     * A statically configured strength for this sensor. See the SensorStrength interface for more
     * information.
     */
    SensorStrength sensorStrength = SensorStrength.CONVENIENCE;

    /**
     * The maximum number of enrollments that a single user can have. Statically configured.
     */
    int maxEnrollmentsPerUser;

    /**
     * A list of component information for subsystems that pertain to this biometric sensor.
     */
    ComponentInfo[] componentInfo;
}
