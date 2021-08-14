/*
 * Copyright 2019 The Android Open Source Project
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

package android.hardware.graphics.common;
@Backing(type="long") @VintfStability
enum BufferUsage {
  CPU_READ_MASK = 15,
  CPU_READ_NEVER = 0,
  CPU_READ_RARELY = 2,
  CPU_READ_OFTEN = 3,
  CPU_WRITE_MASK = 240,
  CPU_WRITE_NEVER = 0,
  CPU_WRITE_RARELY = 32,
  CPU_WRITE_OFTEN = 48,
  GPU_TEXTURE = 256,
  GPU_RENDER_TARGET = 512,
  COMPOSER_OVERLAY = 2048,
  COMPOSER_CLIENT_TARGET = 4096,
  PROTECTED = 16384,
  COMPOSER_CURSOR = 32768,
  VIDEO_ENCODER = 65536,
  CAMERA_OUTPUT = 131072,
  CAMERA_INPUT = 262144,
  RENDERSCRIPT = 1048576,
  VIDEO_DECODER = 4194304,
  SENSOR_DIRECT_DATA = 8388608,
  GPU_CUBE_MAP = 33554432,
  GPU_MIPMAP_COMPLETE = 67108864,
  HW_IMAGE_ENCODER = 134217728,
  GPU_DATA_BUFFER = 16777216,
  VENDOR_MASK = -268435456,
  VENDOR_MASK_HI = -281474976710656,
}
