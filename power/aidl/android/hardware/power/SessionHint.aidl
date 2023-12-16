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

package android.hardware.power;

@VintfStability
@Backing(type="int")
enum SessionHint {
    /**
     * This hint indicates an increase in CPU workload intensity. It means that
     * this hint session needs extra CPU resources to meet the target duration.
     * This hint must be sent before reporting the actual duration to the session.
     */
    CPU_LOAD_UP = 0,

    /**
     * This hint indicates a decrease in CPU workload intensity. It means that
     * this hint session can reduce CPU resources and still meet the target duration.
     */
    CPU_LOAD_DOWN = 1,

    /**
     * This hint indicates an upcoming CPU workload that is completely changed and
     * unknown. It means that the hint session should reset CPU resources to a known
     * baseline to prepare for an arbitrary load, and must wake up if inactive.
     */
    CPU_LOAD_RESET = 2,

    /**
     * This hint indicates that the most recent CPU workload is resuming after a
     * period of inactivity. It means that the hint session should allocate similar
     * CPU resources to what was used previously, and must wake up if inactive.
     */
    CPU_LOAD_RESUME = 3,

    /**
     * This hint indicates that this power hint session should be applied with a
     * power-efficient-first scheduling strategy. This means the work of this
     * power hint session is noncritical despite its CPU intensity.
     */
    POWER_EFFICIENCY = 4,

    /**
     * This hint indicates an increase in GPU workload intensity. It means that
     * this hint session needs extra GPU resources to meet the target duration.
     * This hint must be sent before reporting the actual duration to the session.
     */
    GPU_LOAD_UP = 5,

    /**
     * This hint indicates a decrease in GPU workload intensity. It means that
     * this hint session can reduce GPU resources and still meet the target duration.
     */
    GPU_LOAD_DOWN = 6,
}
