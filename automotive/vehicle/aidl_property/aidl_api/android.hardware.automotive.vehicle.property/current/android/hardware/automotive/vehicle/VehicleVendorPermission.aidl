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
enum VehicleVendorPermission {
  PERMISSION_DEFAULT = 0x00000000,
  PERMISSION_SET_VENDOR_CATEGORY_WINDOW = 0X00000001,
  PERMISSION_GET_VENDOR_CATEGORY_WINDOW = 0x00000002,
  PERMISSION_SET_VENDOR_CATEGORY_DOOR = 0x00000003,
  PERMISSION_GET_VENDOR_CATEGORY_DOOR = 0x00000004,
  PERMISSION_SET_VENDOR_CATEGORY_SEAT = 0x00000005,
  PERMISSION_GET_VENDOR_CATEGORY_SEAT = 0x00000006,
  PERMISSION_SET_VENDOR_CATEGORY_MIRROR = 0x00000007,
  PERMISSION_GET_VENDOR_CATEGORY_MIRROR = 0x00000008,
  PERMISSION_SET_VENDOR_CATEGORY_INFO = 0x00000009,
  PERMISSION_GET_VENDOR_CATEGORY_INFO = 0x0000000A,
  PERMISSION_SET_VENDOR_CATEGORY_ENGINE = 0x0000000B,
  PERMISSION_GET_VENDOR_CATEGORY_ENGINE = 0x0000000C,
  PERMISSION_SET_VENDOR_CATEGORY_HVAC = 0x0000000D,
  PERMISSION_GET_VENDOR_CATEGORY_HVAC = 0x0000000E,
  PERMISSION_SET_VENDOR_CATEGORY_LIGHT = 0x0000000F,
  PERMISSION_GET_VENDOR_CATEGORY_LIGHT = 0x00000010,
  PERMISSION_SET_VENDOR_CATEGORY_1 = 0x00010000,
  PERMISSION_GET_VENDOR_CATEGORY_1 = 0x00011000,
  PERMISSION_SET_VENDOR_CATEGORY_2 = 0x00020000,
  PERMISSION_GET_VENDOR_CATEGORY_2 = 0x00021000,
  PERMISSION_SET_VENDOR_CATEGORY_3 = 0x00030000,
  PERMISSION_GET_VENDOR_CATEGORY_3 = 0x00031000,
  PERMISSION_SET_VENDOR_CATEGORY_4 = 0x00040000,
  PERMISSION_GET_VENDOR_CATEGORY_4 = 0x00041000,
  PERMISSION_SET_VENDOR_CATEGORY_5 = 0x00050000,
  PERMISSION_GET_VENDOR_CATEGORY_5 = 0x00051000,
  PERMISSION_SET_VENDOR_CATEGORY_6 = 0x00060000,
  PERMISSION_GET_VENDOR_CATEGORY_6 = 0x00061000,
  PERMISSION_SET_VENDOR_CATEGORY_7 = 0x00070000,
  PERMISSION_GET_VENDOR_CATEGORY_7 = 0x00071000,
  PERMISSION_SET_VENDOR_CATEGORY_8 = 0x00080000,
  PERMISSION_GET_VENDOR_CATEGORY_8 = 0x00081000,
  PERMISSION_SET_VENDOR_CATEGORY_9 = 0x00090000,
  PERMISSION_GET_VENDOR_CATEGORY_9 = 0x00091000,
  PERMISSION_SET_VENDOR_CATEGORY_10 = 0x000A0000,
  PERMISSION_GET_VENDOR_CATEGORY_10 = 0x000A1000,
  PERMISSION_NOT_ACCESSIBLE = 0xF0000000,
}
