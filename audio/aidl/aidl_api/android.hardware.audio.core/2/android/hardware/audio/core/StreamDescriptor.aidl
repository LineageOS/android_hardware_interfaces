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

package android.hardware.audio.core;
@JavaDerive(equals=true, toString=true) @VintfStability
parcelable StreamDescriptor {
  android.hardware.common.fmq.MQDescriptor<android.hardware.audio.core.StreamDescriptor.Command,android.hardware.common.fmq.SynchronizedReadWrite> command;
  android.hardware.common.fmq.MQDescriptor<android.hardware.audio.core.StreamDescriptor.Reply,android.hardware.common.fmq.SynchronizedReadWrite> reply;
  int frameSizeBytes;
  long bufferSizeFrames;
  android.hardware.audio.core.StreamDescriptor.AudioBuffer audio;
  const int LATENCY_UNKNOWN = (-1) /* -1 */;
  @FixedSize @VintfStability
  parcelable Position {
    long frames = UNKNOWN /* -1 */;
    long timeNs = UNKNOWN /* -1 */;
    const long UNKNOWN = (-1) /* -1 */;
  }
  @Backing(type="int") @VintfStability
  enum State {
    STANDBY = 1,
    IDLE = 2,
    ACTIVE = 3,
    PAUSED = 4,
    DRAINING = 5,
    DRAIN_PAUSED = 6,
    TRANSFERRING = 7,
    TRANSFER_PAUSED = 8,
    ERROR = 100,
  }
  @Backing(type="byte") @VintfStability
  enum DrainMode {
    DRAIN_UNSPECIFIED = 0,
    DRAIN_ALL = 1,
    DRAIN_EARLY_NOTIFY = 2,
  }
  @FixedSize @VintfStability
  union Command {
    int halReservedExit;
    android.media.audio.common.Void getStatus;
    android.media.audio.common.Void start;
    int burst;
    android.hardware.audio.core.StreamDescriptor.DrainMode drain;
    android.media.audio.common.Void standby;
    android.media.audio.common.Void pause;
    android.media.audio.common.Void flush;
  }
  @FixedSize @VintfStability
  parcelable Reply {
    int status;
    int fmqByteCount;
    android.hardware.audio.core.StreamDescriptor.Position observable;
    android.hardware.audio.core.StreamDescriptor.Position hardware;
    int latencyMs;
    int xrunFrames;
    android.hardware.audio.core.StreamDescriptor.State state = android.hardware.audio.core.StreamDescriptor.State.STANDBY;
  }
  @VintfStability
  union AudioBuffer {
    android.hardware.common.fmq.MQDescriptor<byte,android.hardware.common.fmq.SynchronizedReadWrite> fmq;
    android.hardware.audio.core.MmapBufferDescriptor mmap;
  }
}
