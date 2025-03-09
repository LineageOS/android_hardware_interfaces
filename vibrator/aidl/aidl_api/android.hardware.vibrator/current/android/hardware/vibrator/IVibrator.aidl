/*
 * Copyright (C) 2019 The Android Open Source Project
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

package android.hardware.vibrator;
@VintfStability
interface IVibrator {
  int getCapabilities();
  void off();
  void on(in int timeoutMs, in android.hardware.vibrator.IVibratorCallback callback);
  int perform(in android.hardware.vibrator.Effect effect, in android.hardware.vibrator.EffectStrength strength, in android.hardware.vibrator.IVibratorCallback callback);
  android.hardware.vibrator.Effect[] getSupportedEffects();
  void setAmplitude(in float amplitude);
  void setExternalControl(in boolean enabled);
  int getCompositionDelayMax();
  int getCompositionSizeMax();
  android.hardware.vibrator.CompositePrimitive[] getSupportedPrimitives();
  int getPrimitiveDuration(android.hardware.vibrator.CompositePrimitive primitive);
  void compose(in android.hardware.vibrator.CompositeEffect[] composite, in android.hardware.vibrator.IVibratorCallback callback);
  android.hardware.vibrator.Effect[] getSupportedAlwaysOnEffects();
  void alwaysOnEnable(in int id, in android.hardware.vibrator.Effect effect, in android.hardware.vibrator.EffectStrength strength);
  void alwaysOnDisable(in int id);
  float getResonantFrequency();
  float getQFactor();
  /**
   * @deprecated This method is deprecated from AIDL v3 and is no longer required to be implemented even if CAP_FREQUENCY_CONTROL capability is reported.
   */
  float getFrequencyResolution();
  /**
   * @deprecated This method is deprecated from AIDL v3 and is no longer required to be implemented even if CAP_FREQUENCY_CONTROL capability is reported.
   */
  float getFrequencyMinimum();
  /**
   * @deprecated This method is deprecated from AIDL v3 and is no longer required to be implemented even if CAP_FREQUENCY_CONTROL capability is reported.
   */
  float[] getBandwidthAmplitudeMap();
  /**
   * @deprecated This method is deprecated from AIDL v3 and is no longer required to be implemented. Use `IVibrator.getPwleV2PrimitiveDurationMaxMillis` instead.
   */
  int getPwlePrimitiveDurationMax();
  /**
   * @deprecated This method is deprecated from AIDL v3 and is no longer required to be implemented. Use `IVibrator.getPwleV2CompositionSizeMax` instead.
   */
  int getPwleCompositionSizeMax();
  /**
   * @deprecated This method is deprecated from AIDL v3 and is no longer required to be implemented.
   */
  android.hardware.vibrator.Braking[] getSupportedBraking();
  /**
   * @deprecated This method is deprecated from AIDL v3 and is no longer required to be implemented. Use `IVibrator.composePwleV2` instead.
   */
  void composePwle(in android.hardware.vibrator.PrimitivePwle[] composite, in android.hardware.vibrator.IVibratorCallback callback);
  void performVendorEffect(in android.hardware.vibrator.VendorEffect vendorEffect, in android.hardware.vibrator.IVibratorCallback callback);
  List<android.hardware.vibrator.FrequencyAccelerationMapEntry> getFrequencyToOutputAccelerationMap();
  int getPwleV2PrimitiveDurationMaxMillis();
  int getPwleV2CompositionSizeMax();
  int getPwleV2PrimitiveDurationMinMillis();
  void composePwleV2(in android.hardware.vibrator.CompositePwleV2 composite, in android.hardware.vibrator.IVibratorCallback callback);
  const int CAP_ON_CALLBACK = (1 << 0) /* 1 */;
  const int CAP_PERFORM_CALLBACK = (1 << 1) /* 2 */;
  const int CAP_AMPLITUDE_CONTROL = (1 << 2) /* 4 */;
  const int CAP_EXTERNAL_CONTROL = (1 << 3) /* 8 */;
  const int CAP_EXTERNAL_AMPLITUDE_CONTROL = (1 << 4) /* 16 */;
  const int CAP_COMPOSE_EFFECTS = (1 << 5) /* 32 */;
  const int CAP_ALWAYS_ON_CONTROL = (1 << 6) /* 64 */;
  const int CAP_GET_RESONANT_FREQUENCY = (1 << 7) /* 128 */;
  const int CAP_GET_Q_FACTOR = (1 << 8) /* 256 */;
  const int CAP_FREQUENCY_CONTROL = (1 << 9) /* 512 */;
  const int CAP_COMPOSE_PWLE_EFFECTS = (1 << 10) /* 1024 */;
  const int CAP_PERFORM_VENDOR_EFFECTS = (1 << 11) /* 2048 */;
  const int CAP_COMPOSE_PWLE_EFFECTS_V2 = (1 << 12) /* 4096 */;
}
