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
 * Calls to neural networks AIDL interfaces may return a ServiceSpecificException with the following
 * error codes.
 */
@VintfStability
@Backing(type="int")
enum ErrorStatus {
    NONE,
    DEVICE_UNAVAILABLE,
    GENERAL_FAILURE,
    OUTPUT_INSUFFICIENT_SIZE,
    INVALID_ARGUMENT,
    /**
     * Failure because a deadline could not be met for a task, but future deadlines may still be met
     * for the same task after a short delay.
     */
    MISSED_DEADLINE_TRANSIENT,
    /**
     * Failure because a deadline could not be met for a task, and future deadlines will likely also
     * not be met for the same task even after a short delay.
     */
    MISSED_DEADLINE_PERSISTENT,
    /**
     * Failure because of a resource limitation within the driver, but future calls for the same
     * task may still succeed after a short delay.
     */
    RESOURCE_EXHAUSTED_TRANSIENT,
    /**
     * Failure because of a resource limitation within the driver, and future calls for the same
     * task will likely also fail even after a short delay.
     */
    RESOURCE_EXHAUSTED_PERSISTENT,
}
