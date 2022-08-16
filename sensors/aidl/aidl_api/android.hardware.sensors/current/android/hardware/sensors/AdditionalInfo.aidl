/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.sensors;
@FixedSize @VintfStability
parcelable AdditionalInfo {
  android.hardware.sensors.AdditionalInfo.AdditionalInfoType type;
  int serial;
  android.hardware.sensors.AdditionalInfo.AdditionalInfoPayload payload;
  @FixedSize @VintfStability
  union AdditionalInfoPayload {
    android.hardware.sensors.AdditionalInfo.AdditionalInfoPayload.Int32Values dataInt32;
    android.hardware.sensors.AdditionalInfo.AdditionalInfoPayload.FloatValues dataFloat;
    @FixedSize @VintfStability
    parcelable Int32Values {
      int[14] values;
    }
    @FixedSize @VintfStability
    parcelable FloatValues {
      float[14] values;
    }
  }
  @Backing(type="int") @VintfStability
  enum AdditionalInfoType {
    AINFO_BEGIN = 0,
    AINFO_END = 1,
    AINFO_UNTRACKED_DELAY = 65536,
    AINFO_INTERNAL_TEMPERATURE = 65537,
    AINFO_VEC3_CALIBRATION = 65538,
    AINFO_SENSOR_PLACEMENT = 65539,
    AINFO_SAMPLING = 65540,
    AINFO_CHANNEL_NOISE = 131072,
    AINFO_CHANNEL_SAMPLER = 131073,
    AINFO_CHANNEL_FILTER = 131074,
    AINFO_CHANNEL_LINEAR_TRANSFORM = 131075,
    AINFO_CHANNEL_NONLINEAR_MAP = 131076,
    AINFO_CHANNEL_RESAMPLER = 131077,
    AINFO_LOCAL_GEOMAGNETIC_FIELD = 196608,
    AINFO_LOCAL_GRAVITY = 196609,
    AINFO_DOCK_STATE = 196610,
    AINFO_HIGH_PERFORMANCE_MODE = 196611,
    AINFO_MAGNETIC_FIELD_CALIBRATION = 196612,
    AINFO_CUSTOM_START = 268435456,
    AINFO_DEBUGGING_START = 1073741824,
  }
}
