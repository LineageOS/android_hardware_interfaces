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

package android.hardware.neuralnetworks;

/**
 * Timing information measured during execution. Each time is a duration from the beginning of some
 * task to the end of that task, including time when that task is not active (for example, preempted
 * by some other task, or waiting for some resource to become available).
 *
 * Times are measured in nanoseconds. When a time is not available, it must be reported as -1.
 */
@VintfStability
parcelable Timing {
    /**
     * Execution time on device (not driver, which runs on host processor).
     */
    long timeOnDeviceNs;
    /**
     * Execution time in driver (including time on device).
     */
    long timeInDriverNs;
}
