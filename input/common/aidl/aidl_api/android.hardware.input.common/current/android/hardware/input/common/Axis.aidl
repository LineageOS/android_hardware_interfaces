/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.input.common;
@Backing(type="int") @VintfStability
enum Axis {
  X = 0,
  Y = 1,
  PRESSURE = 2,
  SIZE = 3,
  TOUCH_MAJOR = 4,
  TOUCH_MINOR = 5,
  TOOL_MAJOR = 6,
  TOOL_MINOR = 7,
  ORIENTATION = 8,
  VSCROLL = 9,
  HSCROLL = 10,
  Z = 11,
  RX = 12,
  RY = 13,
  RZ = 14,
  HAT_X = 15,
  HAT_Y = 16,
  LTRIGGER = 17,
  RTRIGGER = 18,
  THROTTLE = 19,
  RUDDER = 20,
  WHEEL = 21,
  GAS = 22,
  BRAKE = 23,
  DISTANCE = 24,
  TILT = 25,
  SCROLL = 26,
  RELATIVE_X = 27,
  RELATIVE_Y = 28,
  GENERIC_1 = 32,
  GENERIC_2 = 33,
  GENERIC_3 = 34,
  GENERIC_4 = 35,
  GENERIC_5 = 36,
  GENERIC_6 = 37,
  GENERIC_7 = 38,
  GENERIC_8 = 39,
  GENERIC_9 = 40,
  GENERIC_10 = 41,
  GENERIC_11 = 42,
  GENERIC_12 = 43,
  GENERIC_13 = 44,
  GENERIC_14 = 45,
  GENERIC_15 = 46,
  GENERIC_16 = 47,
}
