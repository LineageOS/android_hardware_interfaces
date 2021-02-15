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
 * Performance information for the reference workload.
 *
 * Used by a driver to report its performance characteristics.
 */
@VintfStability
parcelable PerformanceInfo {
    /**
     * Ratio of the time taken by the driver to execute the workload compared to the time the CPU
     * would take for the same workload. A lower number is better.
     */
    float execTime;
    /**
     * Ratio of the energy used by the driver compared to what the CPU would use for doing the same
     * workload. A lower number is better.
     */
    float powerUsage;
}
