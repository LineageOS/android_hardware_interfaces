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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.automotive.vehicle;
@Backing(type="int") @VintfStability
enum VehicleUnit {
  SHOULD_NOT_USE = 0,
  METER_PER_SEC = 1,
  RPM = 2,
  HERTZ = 3,
  PERCENTILE = 16,
  MILLIMETER = 32,
  METER = 33,
  KILOMETER = 35,
  MILE = 36,
  CELSIUS = 48,
  FAHRENHEIT = 49,
  KELVIN = 50,
  MILLILITER = 64,
  LITER = 65,
  GALLON = 66,
  US_GALLON = 66,
  IMPERIAL_GALLON = 67,
  NANO_SECS = 80,
  SECS = 83,
  YEAR = 89,
  WATT_HOUR = 96,
  MILLIAMPERE = 97,
  MILLIVOLT = 98,
  MILLIWATTS = 99,
  AMPERE_HOURS = 100,
  KILOWATT_HOUR = 101,
  AMPERE = 102,
  KILOPASCAL = 112,
  PSI = 113,
  BAR = 114,
  DEGREES = 128,
  MILES_PER_HOUR = 144,
  KILOMETERS_PER_HOUR = 145,
}
