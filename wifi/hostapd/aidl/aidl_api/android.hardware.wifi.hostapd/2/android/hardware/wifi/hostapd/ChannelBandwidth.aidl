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

package android.hardware.wifi.hostapd;
@Backing(type="int") @VintfStability
enum ChannelBandwidth {
  BANDWIDTH_INVALID = 0,
  BANDWIDTH_AUTO = 1,
  BANDWIDTH_20_NOHT = 2,
  BANDWIDTH_20 = 3,
  BANDWIDTH_40 = 4,
  BANDWIDTH_80 = 5,
  BANDWIDTH_80P80 = 6,
  BANDWIDTH_160 = 7,
  BANDWIDTH_320 = 8,
  BANDWIDTH_2160 = 9,
  BANDWIDTH_4320 = 10,
  BANDWIDTH_6480 = 11,
  BANDWIDTH_8640 = 12,
}
