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
union DynamicsProcessing {
  android.hardware.audio.effect.VendorExtension vendorExtension;
  android.hardware.audio.effect.DynamicsProcessing.EngineArchitecture engineArchitecture;
  android.hardware.audio.effect.DynamicsProcessing.BandChannelConfig preEq;
  android.hardware.audio.effect.DynamicsProcessing.BandChannelConfig postEq;
  android.hardware.audio.effect.DynamicsProcessing.EqBandConfig preEqBand;
  android.hardware.audio.effect.DynamicsProcessing.EqBandConfig postEqBand;
  android.hardware.audio.effect.DynamicsProcessing.BandChannelConfig mbc;
  android.hardware.audio.effect.DynamicsProcessing.MbcBandConfig mbcBand;
  android.hardware.audio.effect.DynamicsProcessing.LimiterConfig limiter;
  float inputGainDb;
  @VintfStability
  union Id {
    int vendorExtensionTag;
    android.hardware.audio.effect.DynamicsProcessing.Tag commonTag;
  }
  @VintfStability
  parcelable Capability {
    ParcelableHolder extension;
  }
  enum ResolutionPreference {
    FAVOR_FREQUENCY_RESOLUTION = 0,
    FAVOR_TIME_RESOLUTION = 1,
  }
  @VintfStability
  parcelable BandEnablement {
    boolean inUse;
    int bandCount;
  }
  @VintfStability
  parcelable EngineArchitecture {
    android.hardware.audio.effect.DynamicsProcessing.ResolutionPreference resolutionPreference = android.hardware.audio.effect.DynamicsProcessing.ResolutionPreference.FAVOR_FREQUENCY_RESOLUTION;
    float preferredFrameDurationMs;
    android.hardware.audio.effect.DynamicsProcessing.BandEnablement preEqBand;
    android.hardware.audio.effect.DynamicsProcessing.BandEnablement postEqBand;
    android.hardware.audio.effect.DynamicsProcessing.BandEnablement mbcBand;
    boolean limiterInUse;
  }
  @VintfStability
  parcelable BandChannelConfig {
    int channel;
    android.hardware.audio.effect.DynamicsProcessing.BandEnablement enablement;
  }
  @VintfStability
  parcelable EqBandConfig {
    int channel;
    int band;
    boolean enable;
    float cutoffFrequency;
    float gain;
  }
  @VintfStability
  parcelable MbcBandConfig {
    int channel;
    int band;
    boolean enable;
    float cutoffFrequencyHz;
    float gainDb;
    float attackTimeMs;
    float releaseTimeMs;
    float ratio;
    float thresholdDb;
    float kneeWidthDb;
    float noiseGateThresholdDb;
    float expanderRatio;
    float preGainDb;
    float postGainDb;
  }
  @VintfStability
  parcelable LimiterConfig {
    int channel;
    boolean enable;
    boolean inUse;
    int linkGroup;
    float attackTimeMs;
    float releaseTimeMs;
    float ratio;
    float thresholdDb;
    float postGainDb;
  }
}
