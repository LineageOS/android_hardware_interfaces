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
 * Used by INFO_FUEL_TYPE to enumerate the type of fuels this vehicle uses.
 * Consistent with projection protocol.
 */
@VintfStability
@Backing(type="int")
enum FuelType {
    /**
     * Fuel type to use if the HU does not know on which types of fuel the vehicle
     * runs. The use of this value is generally discouraged outside of aftermarket units.
     */
    FUEL_TYPE_UNKNOWN = 0,
    /**
     * Unleaded gasoline
     */
    FUEL_TYPE_UNLEADED = 1,
    /**
     * Leaded gasoline
     */
    FUEL_TYPE_LEADED = 2,
    /**
     * Diesel #1
     */
    FUEL_TYPE_DIESEL_1 = 3,
    /**
     * Diesel #2
     */
    FUEL_TYPE_DIESEL_2 = 4,
    /**
     * Biodiesel
     */
    FUEL_TYPE_BIODIESEL = 5,
    /**
     * 85% ethanol/gasoline blend
     */
    FUEL_TYPE_E85 = 6,
    /**
     * Liquified petroleum gas
     */
    FUEL_TYPE_LPG = 7,
    /**
     * Compressed natural gas
     */
    FUEL_TYPE_CNG = 8,
    /**
     * Liquified natural gas
     */
    FUEL_TYPE_LNG = 9,
    /**
     * Electric
     */
    FUEL_TYPE_ELECTRIC = 10,
    /**
     * Hydrogen fuel cell
     */
    FUEL_TYPE_HYDROGEN = 11,
    /**
     * Fuel type to use when no other types apply. Before using this value, work with
     * Google to see if the FuelType enum can be extended with an appropriate value.
     */
    FUEL_TYPE_OTHER = 12,
}
