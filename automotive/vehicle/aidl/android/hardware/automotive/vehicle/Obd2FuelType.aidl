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
 * The fuel type(s) supported by a vehicle.
 * These values come from the SAE J1979 standard.
 */
@VintfStability
@Backing(type="int")
enum Obd2FuelType {
    NOT_AVAILABLE = 0,
    GASOLINE = 1,
    METHANOL = 2,
    ETHANOL = 3,
    DIESEL = 4,
    LPG = 5,
    CNG = 6,
    PROPANE = 7,
    ELECTRIC = 8,
    BIFUEL_RUNNING_GASOLINE = 9,
    BIFUEL_RUNNING_METHANOL = 10,
    BIFUEL_RUNNING_ETHANOL = 11,
    BIFUEL_RUNNING_LPG = 12,
    BIFUEL_RUNNING_CNG = 13,
    BIFUEL_RUNNING_PROPANE = 14,
    BIFUEL_RUNNING_ELECTRIC = 15,
    BIFUEL_RUNNING_ELECTRIC_AND_COMBUSTION = 16,
    HYBRID_GASOLINE = 17,
    HYBRID_ETHANOL = 18,
    HYBRID_DIESEL = 19,
    HYBRID_ELECTRIC = 20,
    HYBRID_RUNNING_ELECTRIC_AND_COMBUSTION = 21,
    HYBRID_REGENERATIVE = 22,
    BIFUEL_RUNNING_DIESEL = 23,
}
