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
interface ISensors {
  void activate(in int sensorHandle, in boolean enabled);
  void batch(in int sensorHandle, in long samplingPeriodNs, in long maxReportLatencyNs);
  int configDirectReport(in int sensorHandle, in int channelHandle, in android.hardware.sensors.ISensors.RateLevel rate);
  void flush(in int sensorHandle);
  android.hardware.sensors.SensorInfo[] getSensorsList();
  void initialize(in android.hardware.common.fmq.MQDescriptor<android.hardware.sensors.Event,android.hardware.common.fmq.SynchronizedReadWrite> eventQueueDescriptor, in android.hardware.common.fmq.MQDescriptor<int,android.hardware.common.fmq.SynchronizedReadWrite> wakeLockDescriptor, in android.hardware.sensors.ISensorsCallback sensorsCallback);
  void injectSensorData(in android.hardware.sensors.Event event);
  int registerDirectChannel(in android.hardware.sensors.ISensors.SharedMemInfo mem);
  void setOperationMode(in android.hardware.sensors.ISensors.OperationMode mode);
  void unregisterDirectChannel(in int channelHandle);
  const int ERROR_NO_MEMORY = -12;
  const int ERROR_BAD_VALUE = -22;
  const int WAKE_LOCK_TIMEOUT_SECONDS = 1;
  const int EVENT_QUEUE_FLAG_BITS_READ_AND_PROCESS = 1;
  const int EVENT_QUEUE_FLAG_BITS_EVENTS_READ = 2;
  const int WAKE_LOCK_QUEUE_FLAG_BITS_DATA_WRITTEN = 1;
  const int DIRECT_REPORT_SENSOR_EVENT_OFFSET_SIZE_FIELD = 0;
  const int DIRECT_REPORT_SENSOR_EVENT_OFFSET_SIZE_REPORT_TOKEN = 4;
  const int DIRECT_REPORT_SENSOR_EVENT_OFFSET_SIZE_SENSOR_TYPE = 8;
  const int DIRECT_REPORT_SENSOR_EVENT_OFFSET_SIZE_ATOMIC_COUNTER = 12;
  const int DIRECT_REPORT_SENSOR_EVENT_OFFSET_SIZE_TIMESTAMP = 16;
  const int DIRECT_REPORT_SENSOR_EVENT_OFFSET_SIZE_DATA = 24;
  const int DIRECT_REPORT_SENSOR_EVENT_OFFSET_SIZE_RESERVED = 88;
  const int DIRECT_REPORT_SENSOR_EVENT_TOTAL_LENGTH = 104;
  const int RUNTIME_SENSORS_HANDLE_BASE = 1593835520;
  const int RUNTIME_SENSORS_HANDLE_END = 1610612735;
  @Backing(type="int") @VintfStability
  enum RateLevel {
    STOP = 0,
    NORMAL = 1,
    FAST = 2,
    VERY_FAST = 3,
  }
  @Backing(type="int") @VintfStability
  enum OperationMode {
    NORMAL = 0,
    DATA_INJECTION = 1,
  }
  @VintfStability
  parcelable SharedMemInfo {
    android.hardware.sensors.ISensors.SharedMemInfo.SharedMemType type;
    android.hardware.sensors.ISensors.SharedMemInfo.SharedMemFormat format;
    int size;
    android.hardware.common.NativeHandle memoryHandle;
    @Backing(type="int") @VintfStability
    enum SharedMemFormat {
      SENSORS_EVENT = 1,
    }
    @Backing(type="int") @VintfStability
    enum SharedMemType {
      ASHMEM = 1,
      GRALLOC = 2,
    }
  }
}
