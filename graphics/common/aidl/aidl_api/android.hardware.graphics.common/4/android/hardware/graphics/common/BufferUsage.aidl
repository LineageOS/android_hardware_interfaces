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
/* @hide */
@Backing(type="long") @VintfStability
enum BufferUsage {
  CPU_READ_MASK = 0xf,
  CPU_READ_NEVER = 0,
  CPU_READ_RARELY = 2,
  CPU_READ_OFTEN = 3,
  CPU_WRITE_MASK = (0xf << 4) /* 240 */,
  CPU_WRITE_NEVER = (0 << 4) /* 0 */,
  CPU_WRITE_RARELY = (2 << 4) /* 32 */,
  CPU_WRITE_OFTEN = (3 << 4) /* 48 */,
  GPU_TEXTURE = (1 << 8) /* 256 */,
  GPU_RENDER_TARGET = (1 << 9) /* 512 */,
  COMPOSER_OVERLAY = (1 << 11) /* 2048 */,
  COMPOSER_CLIENT_TARGET = (1 << 12) /* 4096 */,
  PROTECTED = (1 << 14) /* 16384 */,
  COMPOSER_CURSOR = (1 << 15) /* 32768 */,
  VIDEO_ENCODER = (1 << 16) /* 65536 */,
  CAMERA_OUTPUT = (1 << 17) /* 131072 */,
  CAMERA_INPUT = (1 << 18) /* 262144 */,
  RENDERSCRIPT = (1 << 20) /* 1048576 */,
  VIDEO_DECODER = (1 << 22) /* 4194304 */,
  SENSOR_DIRECT_DATA = (1 << 23) /* 8388608 */,
  GPU_DATA_BUFFER = (1 << 24) /* 16777216 */,
  GPU_CUBE_MAP = (1 << 25) /* 33554432 */,
  GPU_MIPMAP_COMPLETE = (1 << 26) /* 67108864 */,
  HW_IMAGE_ENCODER = (1 << 27) /* 134217728 */,
  FRONT_BUFFER = (1L << 32) /* 4294967296 */,
  VENDOR_MASK = (0xf << 28) /* -268435456 */,
  VENDOR_MASK_HI = ((1L * 0xffff) << 48) /* -281474976710656 */,
}
