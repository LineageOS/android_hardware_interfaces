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

package android.hardware.broadcastradio;
@VintfStability
interface IBroadcastRadio {
  android.hardware.broadcastradio.Properties getProperties();
  android.hardware.broadcastradio.AmFmRegionConfig getAmFmRegionConfig(in boolean full);
  android.hardware.broadcastradio.DabTableEntry[] getDabRegionConfig();
  void setTunerCallback(in android.hardware.broadcastradio.ITunerCallback callback);
  void unsetTunerCallback();
  void tune(in android.hardware.broadcastradio.ProgramSelector program);
  void seek(in boolean directionUp, in boolean skipSubChannel);
  void step(in boolean directionUp);
  void cancel();
  void startProgramListUpdates(in android.hardware.broadcastradio.ProgramFilter filter);
  void stopProgramListUpdates();
  boolean isConfigFlagSet(in android.hardware.broadcastradio.ConfigFlag flag);
  void setConfigFlag(in android.hardware.broadcastradio.ConfigFlag flag, in boolean value);
  android.hardware.broadcastradio.VendorKeyValue[] setParameters(in android.hardware.broadcastradio.VendorKeyValue[] parameters);
  android.hardware.broadcastradio.VendorKeyValue[] getParameters(in String[] keys);
  byte[] getImage(in int id);
  android.hardware.broadcastradio.ICloseHandle registerAnnouncementListener(in android.hardware.broadcastradio.IAnnouncementListener listener, in android.hardware.broadcastradio.AnnouncementType[] enabled);
  const int INVALID_IMAGE = 0;
  const int ANTENNA_STATE_CHANGE_TIMEOUT_MS = 100;
  const int LIST_COMPLETE_TIMEOUT_MS = 300000;
  const int TUNER_TIMEOUT_MS = 30000;
}
