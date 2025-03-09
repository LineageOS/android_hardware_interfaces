/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.power;

@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable GpuHeadroomParams {
    /**
     * Defines how to calculate the headroom.
     */
    enum CalculationType {
        // Default to return the minimum headroom in a window.
        MIN,
        // Returns the average headroom in a window.
        AVERAGE,
    }

    /**
     * The calculation type.
     */
    CalculationType calculationType = CalculationType.MIN;

    /**
     * The device should support a superset of [50, 10000] and try to use the closest feasible
     * window size to the provided value param.
     */
    int calculationWindowMillis = 1000;
}
