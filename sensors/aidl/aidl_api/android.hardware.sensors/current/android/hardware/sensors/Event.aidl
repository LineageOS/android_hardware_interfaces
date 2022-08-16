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
parcelable Event {
  long timestamp;
  int sensorHandle;
  android.hardware.sensors.SensorType sensorType;
  android.hardware.sensors.Event.EventPayload payload;
  @FixedSize @VintfStability
  union EventPayload {
    android.hardware.sensors.Event.EventPayload.Vec3 vec3;
    android.hardware.sensors.Event.EventPayload.Vec4 vec4;
    android.hardware.sensors.Event.EventPayload.Uncal uncal;
    android.hardware.sensors.Event.EventPayload.MetaData meta;
    float scalar;
    long stepCount;
    android.hardware.sensors.Event.EventPayload.HeartRate heartRate;
    android.hardware.sensors.Event.EventPayload.Pose6Dof pose6DOF;
    android.hardware.sensors.DynamicSensorInfo dynamic;
    android.hardware.sensors.AdditionalInfo additional;
    android.hardware.sensors.Event.EventPayload.Data data;
    android.hardware.sensors.Event.EventPayload.HeadTracker headTracker;
    android.hardware.sensors.Event.EventPayload.LimitedAxesImu limitedAxesImu;
    android.hardware.sensors.Event.EventPayload.LimitedAxesImuUncal limitedAxesImuUncal;
    android.hardware.sensors.Event.EventPayload.Heading heading;
    @FixedSize @VintfStability
    parcelable Vec4 {
      float x;
      float y;
      float z;
      float w;
    }
    @FixedSize @VintfStability
    parcelable Vec3 {
      float x;
      float y;
      float z;
      android.hardware.sensors.SensorStatus status;
    }
    @FixedSize @VintfStability
    parcelable Uncal {
      float x;
      float y;
      float z;
      float xBias;
      float yBias;
      float zBias;
    }
    @FixedSize @VintfStability
    parcelable HeadTracker {
      float rx;
      float ry;
      float rz;
      float vx;
      float vy;
      float vz;
      int discontinuityCount;
    }
    @FixedSize @VintfStability
    parcelable LimitedAxesImu {
      float x;
      float y;
      float z;
      float xSupported;
      float ySupported;
      float zSupported;
    }
    @FixedSize @VintfStability
    parcelable LimitedAxesImuUncal {
      float x;
      float y;
      float z;
      float xBias;
      float yBias;
      float zBias;
      float xSupported;
      float ySupported;
      float zSupported;
    }
    @FixedSize @VintfStability
    parcelable HeartRate {
      float bpm;
      android.hardware.sensors.SensorStatus status;
    }
    @FixedSize @VintfStability
    parcelable Heading {
      float heading;
      float accuracy;
    }
    @FixedSize @VintfStability
    parcelable MetaData {
      android.hardware.sensors.Event.EventPayload.MetaData.MetaDataEventType what;
      @Backing(type="int") @VintfStability
      enum MetaDataEventType {
        META_DATA_FLUSH_COMPLETE = 1,
      }
    }
    @FixedSize @VintfStability
    parcelable Pose6Dof {
      float[15] values;
    }
    @FixedSize @VintfStability
    parcelable Data {
      float[16] values;
    }
  }
}
