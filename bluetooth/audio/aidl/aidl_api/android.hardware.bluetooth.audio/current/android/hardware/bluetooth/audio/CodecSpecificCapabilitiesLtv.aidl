/*
 * Copyright 2023 The Android Open Source Project
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

package android.hardware.bluetooth.audio;
@VintfStability
union CodecSpecificCapabilitiesLtv {
  android.hardware.bluetooth.audio.CodecSpecificCapabilitiesLtv.SupportedSamplingFrequencies supportedSamplingFrequencies;
  android.hardware.bluetooth.audio.CodecSpecificCapabilitiesLtv.SupportedFrameDurations supportedFrameDurations;
  android.hardware.bluetooth.audio.CodecSpecificCapabilitiesLtv.SupportedAudioChannelCounts supportedAudioChannelCounts;
  android.hardware.bluetooth.audio.CodecSpecificCapabilitiesLtv.SupportedOctetsPerCodecFrame supportedOctetsPerCodecFrame;
  android.hardware.bluetooth.audio.CodecSpecificCapabilitiesLtv.SupportedMaxCodecFramesPerSDU supportedMaxCodecFramesPerSDU;
  parcelable SupportedSamplingFrequencies {
    int bitmask;
    const int HZ8000 = 0x0001;
    const int HZ11025 = 0x0002;
    const int HZ16000 = 0x0004;
    const int HZ22050 = 0x0008;
    const int HZ24000 = 0x0010;
    const int HZ32000 = 0x0020;
    const int HZ44100 = 0x0040;
    const int HZ48000 = 0x0080;
    const int HZ88200 = 0x0100;
    const int HZ96000 = 0x0200;
    const int HZ176400 = 0x0400;
    const int HZ192000 = 0x0800;
    const int HZ384000 = 0x1000;
  }
  parcelable SupportedFrameDurations {
    int bitmask;
    const int US7500 = 0x01;
    const int US10000 = 0x02;
    const int US7500PREFERRED = 0x10;
    const int US10000PREFERRED = 0x20;
  }
  parcelable SupportedAudioChannelCounts {
    int bitmask;
    const int ONE = 0x01;
    const int TWO = 0x02;
    const int THREE = 0x04;
    const int FOUR = 0x08;
    const int FIVE = 0x10;
    const int SIX = 0x20;
    const int SEVEN = 0x40;
    const int EIGHT = 0x80;
  }
  parcelable SupportedOctetsPerCodecFrame {
    int minimum;
    int maximum;
  }
  parcelable SupportedMaxCodecFramesPerSDU {
    int value;
  }
}
