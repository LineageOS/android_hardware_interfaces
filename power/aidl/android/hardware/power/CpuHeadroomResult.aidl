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

/**
 * Headroom value result depending on the request params.
 *
 * Each value is ranged from [0, 100], where 0 indicates no CPU resources were left
 * during the calculation interval and the app may expect low resources to be granted.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
union CpuHeadroomResult {
    /**
     * If ALL selection type is requested.
     */
    float globalHeadroom;
}
