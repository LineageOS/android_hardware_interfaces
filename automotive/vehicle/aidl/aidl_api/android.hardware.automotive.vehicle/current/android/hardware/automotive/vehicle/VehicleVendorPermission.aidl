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
  PERMISSION_DEFAULT = 0,
  PERMISSION_SET_VENDOR_CATEGORY_WINDOW = 1,
  PERMISSION_GET_VENDOR_CATEGORY_WINDOW = 2,
  PERMISSION_SET_VENDOR_CATEGORY_DOOR = 3,
  PERMISSION_GET_VENDOR_CATEGORY_DOOR = 4,
  PERMISSION_SET_VENDOR_CATEGORY_SEAT = 5,
  PERMISSION_GET_VENDOR_CATEGORY_SEAT = 6,
  PERMISSION_SET_VENDOR_CATEGORY_MIRROR = 7,
  PERMISSION_GET_VENDOR_CATEGORY_MIRROR = 8,
  PERMISSION_SET_VENDOR_CATEGORY_INFO = 9,
  PERMISSION_GET_VENDOR_CATEGORY_INFO = 10,
  PERMISSION_SET_VENDOR_CATEGORY_ENGINE = 11,
  PERMISSION_GET_VENDOR_CATEGORY_ENGINE = 12,
  PERMISSION_SET_VENDOR_CATEGORY_HVAC = 13,
  PERMISSION_GET_VENDOR_CATEGORY_HVAC = 14,
  PERMISSION_SET_VENDOR_CATEGORY_LIGHT = 15,
  PERMISSION_GET_VENDOR_CATEGORY_LIGHT = 16,
  PERMISSION_SET_VENDOR_CATEGORY_1 = 65536,
  PERMISSION_GET_VENDOR_CATEGORY_1 = 69632,
  PERMISSION_SET_VENDOR_CATEGORY_2 = 131072,
  PERMISSION_GET_VENDOR_CATEGORY_2 = 135168,
  PERMISSION_SET_VENDOR_CATEGORY_3 = 196608,
  PERMISSION_GET_VENDOR_CATEGORY_3 = 200704,
  PERMISSION_SET_VENDOR_CATEGORY_4 = 262144,
  PERMISSION_GET_VENDOR_CATEGORY_4 = 266240,
  PERMISSION_SET_VENDOR_CATEGORY_5 = 327680,
  PERMISSION_GET_VENDOR_CATEGORY_5 = 331776,
  PERMISSION_SET_VENDOR_CATEGORY_6 = 393216,
  PERMISSION_GET_VENDOR_CATEGORY_6 = 397312,
  PERMISSION_SET_VENDOR_CATEGORY_7 = 458752,
  PERMISSION_GET_VENDOR_CATEGORY_7 = 462848,
  PERMISSION_SET_VENDOR_CATEGORY_8 = 524288,
  PERMISSION_GET_VENDOR_CATEGORY_8 = 528384,
  PERMISSION_SET_VENDOR_CATEGORY_9 = 589824,
  PERMISSION_GET_VENDOR_CATEGORY_9 = 593920,
  PERMISSION_SET_VENDOR_CATEGORY_10 = 655360,
  PERMISSION_GET_VENDOR_CATEGORY_10 = 659456,
  PERMISSION_NOT_ACCESSIBLE = -268435456,
}
