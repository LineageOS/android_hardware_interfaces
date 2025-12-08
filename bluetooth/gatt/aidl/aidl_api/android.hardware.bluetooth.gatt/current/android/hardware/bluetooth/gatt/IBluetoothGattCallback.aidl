/*
 * Copyright (C) 2025 The Android Open Source Project
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

package android.hardware.bluetooth.gatt;
@VintfStability
interface IBluetoothGattCallback {
  void registerServiceComplete(int sessionId, in android.hardware.bluetooth.gatt.IBluetoothGattCallback.Status status, in String reason);
  void unregisterServiceComplete(int sessionId, in String reason);
  void clearServicesComplete(int aclConnectionHandle, in String reason);
  void errorReport(in int aclConnectionHandle, in int localCid, in android.hardware.bluetooth.gatt.IBluetoothGattCallback.Error error, in String reason);
  @Backing(type="int") @VintfStability
  enum Error {
    UNKNOWN,
    DATABASE_OUT_OF_SYNC,
    RESPONSE_TIMEOUT,
    PROTOCOL_VIOLATION,
  }
  @Backing(type="int") @VintfStability
  enum Status {
    SUCCESS = 0,
    INVALID_ENDPOINT_ID,
    UNSUPPORTED_ROLE,
    INSUFFICIENT_RESOURCES,
    FAILURE,
  }
}
