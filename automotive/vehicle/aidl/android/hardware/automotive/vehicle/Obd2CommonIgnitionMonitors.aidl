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
 * Ignition monitors common to both SPARK and COMPRESSION.
 * These values come from the SAE J1979 standard.
 */
@VintfStability
@Backing(type="int")
enum Obd2CommonIgnitionMonitors {
    COMPONENTS_AVAILABLE = 0x1 << 0,
    COMPONENTS_INCOMPLETE = 0x1 << 1,
    FUEL_SYSTEM_AVAILABLE = 0x1 << 2,
    FUEL_SYSTEM_INCOMPLETE = 0x1 << 3,
    MISFIRE_AVAILABLE = 0x1 << 4,
    MISFIRE_INCOMPLETE = 0x1 << 5,
}
