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
@VintfStability
parcelable SensorInfo {
  int sensorHandle;
  String name;
  String vendor;
  int version;
  android.hardware.sensors.SensorType type;
  String typeAsString;
  float maxRange;
  float resolution;
  float power;
  int minDelayUs;
  int fifoReservedEventCount;
  int fifoMaxEventCount;
  String requiredPermission;
  int maxDelayUs;
  int flags;
  const int SENSOR_FLAG_BITS_WAKE_UP = 1;
  const int SENSOR_FLAG_BITS_CONTINUOUS_MODE = 0;
  const int SENSOR_FLAG_BITS_ON_CHANGE_MODE = 2;
  const int SENSOR_FLAG_BITS_ONE_SHOT_MODE = 4;
  const int SENSOR_FLAG_BITS_SPECIAL_REPORTING_MODE = 6;
  const int SENSOR_FLAG_BITS_DATA_INJECTION = 0x10;
  const int SENSOR_FLAG_BITS_DYNAMIC_SENSOR = 0x20;
  const int SENSOR_FLAG_BITS_ADDITIONAL_INFO = 0x40;
  const int SENSOR_FLAG_BITS_DIRECT_CHANNEL_ASHMEM = 0x400;
  const int SENSOR_FLAG_BITS_DIRECT_CHANNEL_GRALLOC = 0x800;
  const int SENSOR_FLAG_BITS_MASK_REPORTING_MODE = 0xE;
  const int SENSOR_FLAG_BITS_MASK_DIRECT_REPORT = 0x380;
  const int SENSOR_FLAG_BITS_MASK_DIRECT_CHANNEL = 0xC00;
  const int SENSOR_FLAG_SHIFT_REPORTING_MODE = 1;
  const int SENSOR_FLAG_SHIFT_DATA_INJECTION = 4;
  const int SENSOR_FLAG_SHIFT_DYNAMIC_SENSOR = 5;
  const int SENSOR_FLAG_SHIFT_ADDITIONAL_INFO = 6;
  const int SENSOR_FLAG_SHIFT_DIRECT_REPORT = 7;
  const int SENSOR_FLAG_SHIFT_DIRECT_CHANNEL = 10;
}
