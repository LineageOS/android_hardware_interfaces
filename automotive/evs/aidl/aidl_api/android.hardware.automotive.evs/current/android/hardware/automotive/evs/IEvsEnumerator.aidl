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

package android.hardware.automotive.evs;
@VintfStability
interface IEvsEnumerator {
  void closeCamera(in android.hardware.automotive.evs.IEvsCamera carCamera);
  void closeDisplay(in android.hardware.automotive.evs.IEvsDisplay display);
  void closeUltrasonicsArray(in android.hardware.automotive.evs.IEvsUltrasonicsArray evsUltrasonicsArray);
  android.hardware.automotive.evs.CameraDesc[] getCameraList();
  byte[] getDisplayIdList();
  android.hardware.automotive.evs.DisplayState getDisplayState();
  android.hardware.automotive.evs.Stream[] getStreamList(in android.hardware.automotive.evs.CameraDesc description);
  android.hardware.automotive.evs.UltrasonicsArrayDesc[] getUltrasonicsArrayList();
  boolean isHardware();
  android.hardware.automotive.evs.IEvsCamera openCamera(in String cameraId, in android.hardware.automotive.evs.Stream streamCfg);
  android.hardware.automotive.evs.IEvsDisplay openDisplay(in int id);
  android.hardware.automotive.evs.IEvsUltrasonicsArray openUltrasonicsArray(in String ultrasonicsArrayId);
  void registerStatusCallback(in android.hardware.automotive.evs.IEvsEnumeratorStatusCallback callback);
  android.hardware.automotive.evs.DisplayState getDisplayStateById(in int id);
}
