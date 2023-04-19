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

package android.hardware.media.c2;
@VintfStability
parcelable Status {
  int status;
  const int OK = 0;
  const int BAD_VALUE = (-22) /* -22 */;
  const int BAD_INDEX = (-75) /* -75 */;
  const int CANNOT_DO = (-2147483646) /* -2147483646 */;
  const int DUPLICATE = (-17) /* -17 */;
  const int NOT_FOUND = (-2) /* -2 */;
  const int BAD_STATE = (-38) /* -38 */;
  const int BLOCKING = (-9930) /* -9930 */;
  const int NO_MEMORY = (-12) /* -12 */;
  const int REFUSED = (-1) /* -1 */;
  const int TIMED_OUT = (-110) /* -110 */;
  const int OMITTED = (-74) /* -74 */;
  const int CORRUPTED = (-2147483648) /* -2147483648 */;
  const int NO_INIT = (-19) /* -19 */;
}
