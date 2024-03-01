/*
 * Copyright (C) 2020 The Android Open Source Project
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
 *//**
 * Important note on Metadata:
 * Metadata qualifies a playback track for an output stream.
 * This is highly closed to {@link android.media.AudioAttributes}.
 * It allows to identify the audio stream rendered / requesting / abandonning the focus.
 *
 * AudioControl 1.0 was limited to identification through {@code AttributeUsage} listed as
 * {@code audioUsage} in audio_policy_configuration.xsd.
 *
 * Any new OEM needs would not be possible without extension.
 *
 * Relying on {@link android.hardware.automotive.audiocontrol.PlaybackTrackMetadata} allows
 * to use a combination of {@code AttributeUsage}, {@code AttributeContentType} and
 * {@code AttributeTags} to identify the use case / routing thanks to
 * {@link android.media.audiopolicy.AudioProductStrategy}.
 * The belonging to a strategy is deduced by an AOSP logic (in sync at native and java layer).
 *
 * IMPORTANT NOTE ON TAGS:
 * To limit the possibilies and prevent from confusion, we expect the String to follow
 * a given formalism that will be enforced.
 *
 * 1 / By convention, tags shall be a "key=value" pair.
 * Vendor must namespace their tag's key (for example com.google.strategy=VR) to avoid conflicts.
 * vendor specific applications and must be prefixed by "VX_". Vendor must
 *
 * 2 / Tags reported here shall be the same as the tags used to define a given
 * {@link android.media.audiopolicy.AudioProductStrategy} and so in
 * audio_policy_engine_configuration.xml file.
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

package android.hardware.automotive.audiocontrol;
@VintfStability
interface IAudioControl {
  /**
   * @deprecated use {@link android.hardware.audio.common.PlaybackTrackMetadata} instead.
   */
  oneway void onAudioFocusChange(in String usage, in int zoneId, in android.hardware.automotive.audiocontrol.AudioFocusChange focusChange);
  oneway void onDevicesToDuckChange(in android.hardware.automotive.audiocontrol.DuckingInfo[] duckingInfos);
  oneway void onDevicesToMuteChange(in android.hardware.automotive.audiocontrol.MutingInfo[] mutingInfos);
  oneway void registerFocusListener(in android.hardware.automotive.audiocontrol.IFocusListener listener);
  oneway void setBalanceTowardRight(in float value);
  oneway void setFadeTowardFront(in float value);
  oneway void onAudioFocusChangeWithMetaData(in android.hardware.audio.common.PlaybackTrackMetadata playbackMetaData, in int zoneId, in android.hardware.automotive.audiocontrol.AudioFocusChange focusChange);
  oneway void setAudioDeviceGainsChanged(in android.hardware.automotive.audiocontrol.Reasons[] reasons, in android.hardware.automotive.audiocontrol.AudioGainConfigInfo[] gains);
  oneway void registerGainCallback(in android.hardware.automotive.audiocontrol.IAudioGainCallback callback);
  void setModuleChangeCallback(in android.hardware.automotive.audiocontrol.IModuleChangeCallback callback);
  void clearModuleChangeCallback();
}
