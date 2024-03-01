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
  SHOULD_NOT_USE = 0x000,
  METER_PER_SEC = 0x01,
  RPM = 0x02,
  HERTZ = 0x03,
  PERCENTILE = 0x10,
  MILLIMETER = 0x20,
  METER = 0x21,
  KILOMETER = 0x23,
  MILE = 0x24,
  CELSIUS = 0x30,
  FAHRENHEIT = 0x31,
  KELVIN = 0x32,
  MILLILITER = 0x40,
  LITER = 0x41,
  GALLON = 0x42,
  US_GALLON = 0x42,
  IMPERIAL_GALLON = 0x43,
  NANO_SECS = 0x50,
  MILLI_SECS = 0x51,
  SECS = 0x53,
  YEAR = 0x59,
  WATT_HOUR = 0x60,
  MILLIAMPERE = 0x61,
  MILLIVOLT = 0x62,
  MILLIWATTS = 0x63,
  AMPERE_HOURS = 0x64,
  KILOWATT_HOUR = 0x65,
  AMPERE = 0x66,
  KILOPASCAL = 0x70,
  PSI = 0x71,
  BAR = 0x72,
  DEGREES = 0x80,
  MILES_PER_HOUR = 0x90,
  KILOMETERS_PER_HOUR = 0x91,
}
