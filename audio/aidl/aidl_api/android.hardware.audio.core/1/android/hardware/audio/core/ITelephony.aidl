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
interface ITelephony {
  android.media.audio.common.AudioMode[] getSupportedAudioModes();
  void switchAudioMode(android.media.audio.common.AudioMode mode);
  android.hardware.audio.core.ITelephony.TelecomConfig setTelecomConfig(in android.hardware.audio.core.ITelephony.TelecomConfig config);
  @JavaDerive(equals=true, toString=true) @VintfStability
  parcelable TelecomConfig {
    @nullable android.media.audio.common.Float voiceVolume;
    android.hardware.audio.core.ITelephony.TelecomConfig.TtyMode ttyMode = android.hardware.audio.core.ITelephony.TelecomConfig.TtyMode.UNSPECIFIED;
    @nullable android.media.audio.common.Boolean isHacEnabled;
    const int VOICE_VOLUME_MIN = 0;
    const int VOICE_VOLUME_MAX = 1;
    @Backing(type="int") @VintfStability
    enum TtyMode {
      UNSPECIFIED = (-1) /* -1 */,
      OFF = 0,
      FULL = 1,
      HCO = 2,
      VCO = 3,
    }
  }
}
