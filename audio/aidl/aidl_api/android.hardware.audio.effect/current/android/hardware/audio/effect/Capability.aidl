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

package android.hardware.audio.effect;
@VintfStability
union Capability {
  android.hardware.audio.effect.VendorExtension vendorExtension;
  android.hardware.audio.effect.AcousticEchoCanceler.Capability acousticEchoCanceler;
  android.hardware.audio.effect.AutomaticGainControl.Capability automaticGainControl;
  android.hardware.audio.effect.BassBoost.Capability bassBoost;
  android.hardware.audio.effect.Downmix.Capability downmix;
  android.hardware.audio.effect.DynamicsProcessing.Capability dynamicsProcessing;
  android.hardware.audio.effect.EnvironmentalReverb.Capability environmentalReverb;
  android.hardware.audio.effect.Equalizer.Capability equalizer;
  android.hardware.audio.effect.HapticGenerator.Capability hapticGenerator;
  android.hardware.audio.effect.LoudnessEnhancer.Capability loudnessEnhancer;
  android.hardware.audio.effect.NoiseSuppression.Capability noiseSuppression;
  android.hardware.audio.effect.PresetReverb.Capability presetReverb;
  android.hardware.audio.effect.Virtualizer.Capability virtualizer;
  android.hardware.audio.effect.Visualizer.Capability visualizer;
  android.hardware.audio.effect.Volume.Capability volume;
}
