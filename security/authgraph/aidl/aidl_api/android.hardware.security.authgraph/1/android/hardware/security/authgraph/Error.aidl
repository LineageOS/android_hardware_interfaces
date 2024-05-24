/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.security.authgraph;
/* @hide */
@Backing(type="int") @VintfStability
enum Error {
  OK = 0,
  INVALID_PEER_NONCE = (-1) /* -1 */,
  INVALID_PEER_KE_KEY = (-2) /* -2 */,
  INVALID_IDENTITY = (-3) /* -3 */,
  INVALID_CERT_CHAIN = (-4) /* -4 */,
  INVALID_SIGNATURE = (-5) /* -5 */,
  INVALID_KE_KEY = (-6) /* -6 */,
  INVALID_PUB_KEY_IN_KEY = (-7) /* -7 */,
  INVALID_PRIV_KEY_ARC_IN_KEY = (-8) /* -8 */,
  INVALID_SHARED_KEY_ARCS = (-9) /* -9 */,
  MEMORY_ALLOCATION_FAILED = (-10) /* -10 */,
  INCOMPATIBLE_PROTOCOL_VERSION = (-11) /* -11 */,
}
