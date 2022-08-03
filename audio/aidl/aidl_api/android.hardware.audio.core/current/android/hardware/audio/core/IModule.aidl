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

package android.hardware.audio.core;
@VintfStability
interface IModule {
  void setModuleDebug(in android.hardware.audio.core.ModuleDebug debug);
  android.media.audio.common.AudioPort connectExternalDevice(in android.media.audio.common.AudioPort templateIdAndAdditionalData);
  void disconnectExternalDevice(int portId);
  android.hardware.audio.core.AudioPatch[] getAudioPatches();
  android.media.audio.common.AudioPort getAudioPort(int portId);
  android.media.audio.common.AudioPortConfig[] getAudioPortConfigs();
  android.media.audio.common.AudioPort[] getAudioPorts();
  android.hardware.audio.core.AudioRoute[] getAudioRoutes();
  android.hardware.audio.core.AudioRoute[] getAudioRoutesForAudioPort(int portId);
  android.hardware.audio.core.IModule.OpenInputStreamReturn openInputStream(in android.hardware.audio.core.IModule.OpenInputStreamArguments args);
  android.hardware.audio.core.IModule.OpenOutputStreamReturn openOutputStream(in android.hardware.audio.core.IModule.OpenOutputStreamArguments args);
  android.hardware.audio.core.AudioPatch setAudioPatch(in android.hardware.audio.core.AudioPatch requested);
  boolean setAudioPortConfig(in android.media.audio.common.AudioPortConfig requested, out android.media.audio.common.AudioPortConfig suggested);
  void resetAudioPatch(int patchId);
  void resetAudioPortConfig(int portConfigId);
  @VintfStability
  parcelable OpenInputStreamArguments {
    int portConfigId;
    android.hardware.audio.common.SinkMetadata sinkMetadata;
    long bufferSizeFrames;
  }
  @VintfStability
  parcelable OpenInputStreamReturn {
    android.hardware.audio.core.IStreamIn stream;
    android.hardware.audio.core.StreamDescriptor desc;
  }
  @VintfStability
  parcelable OpenOutputStreamArguments {
    int portConfigId;
    android.hardware.audio.common.SourceMetadata sourceMetadata;
    @nullable android.media.audio.common.AudioOffloadInfo offloadInfo;
    long bufferSizeFrames;
  }
  @VintfStability
  parcelable OpenOutputStreamReturn {
    android.hardware.audio.core.IStreamOut stream;
    android.hardware.audio.core.StreamDescriptor desc;
  }
}
