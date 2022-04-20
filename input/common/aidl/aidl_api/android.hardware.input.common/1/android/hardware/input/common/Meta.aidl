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
enum Meta {
  NONE = 0,
  ALT_ON = 2,
  ALT_LEFT_ON = 16,
  ALT_RIGHT_ON = 32,
  SHIFT_ON = 1,
  SHIFT_LEFT_ON = 64,
  SHIFT_RIGHT_ON = 128,
  SYM_ON = 4,
  FUNCTION_ON = 8,
  CTRL_ON = 4096,
  CTRL_LEFT_ON = 8192,
  CTRL_RIGHT_ON = 16384,
  META_ON = 65536,
  META_LEFT_ON = 131072,
  META_RIGHT_ON = 262144,
  CAPS_LOCK_ON = 1048576,
  NUM_LOCK_ON = 2097152,
  SCROLL_LOCK_ON = 4194304,
}
