/*
 * Copyright 2023 The Android Open Source Project
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

package android.hardware.bluetooth.audio;
@VintfStability
parcelable AudioContext {
  int bitmask;
  const int UNSPECIFIED = 0x0001;
  const int CONVERSATIONAL = 0x0002;
  const int MEDIA = 0x0004;
  const int GAME = 0x0008;
  const int INSTRUCTIONAL = 0x0010;
  const int VOICE_ASSISTANTS = 0x0020;
  const int LIVE_AUDIO = 0x0040;
  const int SOUND_EFFECTS = 0x0080;
  const int NOTIFICATIONS = 0x0100;
  const int RINGTONE_ALERTS = 0x0200;
  const int ALERTS = 0x0400;
  const int EMERGENCY_ALARM = 0x0800;
}
