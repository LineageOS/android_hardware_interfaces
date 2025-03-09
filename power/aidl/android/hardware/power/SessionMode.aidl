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

package android.hardware.power;

@VintfStability
@Backing(type="int")
enum SessionMode {
    /**
     * This mode indicates that the work of this hint session is not
     * critical to perceived performance, despite its CPU intensity,
     * and can be safely scheduled to prefer power efficiency.
     */
    POWER_EFFICIENCY,

    /**
     * This mode indicates that the threads associated with this hint session
     * are part of the graphics pipeline, implying that they are on a critical path
     * which will be called of higher priority in terms of CPU resources and scheduling.
     */
    GRAPHICS_PIPELINE,

    /**
     * This mode indicates that the session does not intend to report CPU timing
     * information, and that it instead will rely entirely on information from
     * SurfaceFlinger. This mode is only supported for sessions that have
     * GRAPHICS_PIPELINE enabled.
     */
    AUTO_CPU,

    /**
     * This mode indicates that the session does not intend to report GPU timing
     * information, and that it instead will rely entirely on information from
     * SurfaceFlinger. This mode is only supported for sessions that have
     * GRAPHICS_PIPELINE enabled.
     */
    AUTO_GPU,
}
