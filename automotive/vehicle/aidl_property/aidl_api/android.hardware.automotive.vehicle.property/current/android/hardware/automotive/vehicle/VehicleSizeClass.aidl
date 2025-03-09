/*
 * Copyright (C) 2024 The Android Open Source Project
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
enum VehicleSizeClass {
  EPA_TWO_SEATER = 0x100,
  EPA_MINICOMPACT = 0x101,
  EPA_SUBCOMPACT = 0x102,
  EPA_COMPACT = 0x103,
  EPA_MIDSIZE = 0x104,
  EPA_LARGE = 0x105,
  EPA_SMALL_STATION_WAGON = 0x106,
  EPA_MIDSIZE_STATION_WAGON = 0x107,
  EPA_LARGE_STATION_WAGON = 0x108,
  EPA_SMALL_PICKUP_TRUCK = 0x109,
  EPA_STANDARD_PICKUP_TRUCK = 0x10A,
  EPA_VAN = 0x10B,
  EPA_MINIVAN = 0x10C,
  EPA_SMALL_SUV = 0x10D,
  EPA_STANDARD_SUV = 0x10E,
  EU_A_SEGMENT = 0x200,
  EU_B_SEGMENT = 0x201,
  EU_C_SEGMENT = 0x202,
  EU_D_SEGMENT = 0x203,
  EU_E_SEGMENT = 0x204,
  EU_F_SEGMENT = 0x205,
  EU_J_SEGMENT = 0x206,
  EU_M_SEGMENT = 0x207,
  EU_S_SEGMENT = 0x208,
  JPN_KEI = 0x300,
  JPN_SMALL_SIZE = 0x301,
  JPN_NORMAL_SIZE = 0x302,
  US_GVWR_CLASS_1_CV = 0x400,
  US_GVWR_CLASS_2_CV = 0x401,
  US_GVWR_CLASS_3_CV = 0x402,
  US_GVWR_CLASS_4_CV = 0x403,
  US_GVWR_CLASS_5_CV = 0x404,
  US_GVWR_CLASS_6_CV = 0x405,
  US_GVWR_CLASS_7_CV = 0x406,
  US_GVWR_CLASS_8_CV = 0x407,
}
