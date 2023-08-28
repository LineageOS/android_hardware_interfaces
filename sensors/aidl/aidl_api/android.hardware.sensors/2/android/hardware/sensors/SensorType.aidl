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
@Backing(type="int") @VintfStability
enum SensorType {
  META_DATA = 0,
  ACCELEROMETER = 1,
  MAGNETIC_FIELD = 2,
  ORIENTATION = 3,
  GYROSCOPE = 4,
  LIGHT = 5,
  PRESSURE = 6,
  PROXIMITY = 8,
  GRAVITY = 9,
  LINEAR_ACCELERATION = 10,
  ROTATION_VECTOR = 11,
  RELATIVE_HUMIDITY = 12,
  AMBIENT_TEMPERATURE = 13,
  MAGNETIC_FIELD_UNCALIBRATED = 14,
  GAME_ROTATION_VECTOR = 15,
  GYROSCOPE_UNCALIBRATED = 16,
  SIGNIFICANT_MOTION = 17,
  STEP_DETECTOR = 18,
  STEP_COUNTER = 19,
  GEOMAGNETIC_ROTATION_VECTOR = 20,
  HEART_RATE = 21,
  TILT_DETECTOR = 22,
  WAKE_GESTURE = 23,
  GLANCE_GESTURE = 24,
  PICK_UP_GESTURE = 25,
  WRIST_TILT_GESTURE = 26,
  DEVICE_ORIENTATION = 27,
  POSE_6DOF = 28,
  STATIONARY_DETECT = 29,
  MOTION_DETECT = 30,
  HEART_BEAT = 31,
  DYNAMIC_SENSOR_META = 32,
  ADDITIONAL_INFO = 33,
  LOW_LATENCY_OFFBODY_DETECT = 34,
  ACCELEROMETER_UNCALIBRATED = 35,
  HINGE_ANGLE = 36,
  HEAD_TRACKER = 37,
  ACCELEROMETER_LIMITED_AXES = 38,
  GYROSCOPE_LIMITED_AXES = 39,
  ACCELEROMETER_LIMITED_AXES_UNCALIBRATED = 40,
  GYROSCOPE_LIMITED_AXES_UNCALIBRATED = 41,
  HEADING = 42,
  DEVICE_PRIVATE_BASE = 0x10000,
}
