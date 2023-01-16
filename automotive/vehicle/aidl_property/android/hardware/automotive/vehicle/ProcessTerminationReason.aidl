/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.automotive.vehicle;

/**
 * The reason why a process is terminated by car watchdog.
 * This is used with WATCHDOG_TERMINATED_PROCESS property.
 */
@VintfStability
@Backing(type="int")
enum ProcessTerminationReason {
    /**
     * A process doesn't respond to car watchdog within the timeout.
     */
    NOT_RESPONDING = 1,
    /**
     * A process uses more IO operations than what is allowed.
     */
    IO_OVERUSE = 2,
    /**
     * A process uses more memory space than what is allowed.
     */
    MEMORY_OVERUSE = 3,
}
