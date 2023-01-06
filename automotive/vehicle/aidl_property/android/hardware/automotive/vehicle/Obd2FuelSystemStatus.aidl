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
 * The status of the vehicle's fuel system.
 * These values come from the SAE J1979 standard.
 */
@VintfStability
@Backing(type="int")
enum Obd2FuelSystemStatus {
    OPEN_INSUFFICIENT_ENGINE_TEMPERATURE = 1,
    CLOSED_LOOP = 2,
    OPEN_ENGINE_LOAD_OR_DECELERATION = 4,
    OPEN_SYSTEM_FAILURE = 8,
    CLOSED_LOOP_BUT_FEEDBACK_FAULT = 16,
}
