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

package android.hardware.biometrics.face;
@Backing(type="byte") @VintfStability
enum AcquiredInfo {
  UNKNOWN = 0,
  GOOD = 1,
  INSUFFICIENT = 2,
  TOO_BRIGHT = 3,
  TOO_DARK = 4,
  TOO_CLOSE = 5,
  TOO_FAR = 6,
  FACE_TOO_HIGH = 7,
  FACE_TOO_LOW = 8,
  FACE_TOO_RIGHT = 9,
  FACE_TOO_LEFT = 10,
  POOR_GAZE = 11,
  NOT_DETECTED = 12,
  TOO_MUCH_MOTION = 13,
  RECALIBRATE = 14,
  TOO_DIFFERENT = 15,
  TOO_SIMILAR = 16,
  PAN_TOO_EXTREME = 17,
  TILT_TOO_EXTREME = 18,
  ROLL_TOO_EXTREME = 19,
  FACE_OBSCURED = 20,
  START = 21,
  SENSOR_DIRTY = 22,
  VENDOR = 23,
  FIRST_FRAME_RECEIVED = 24,
  DARK_GLASSES_DETECTED = 25,
  MOUTH_COVERING_DETECTED = 26,
}
