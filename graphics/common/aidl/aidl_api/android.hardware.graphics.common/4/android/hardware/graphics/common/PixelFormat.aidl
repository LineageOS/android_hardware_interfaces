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
@Backing(type="int") @VintfStability
enum PixelFormat {
  UNSPECIFIED = 0,
  RGBA_8888 = 0x1,
  RGBX_8888 = 0x2,
  RGB_888 = 0x3,
  RGB_565 = 0x4,
  BGRA_8888 = 0x5,
  YCBCR_422_SP = 0x10,
  YCRCB_420_SP = 0x11,
  YCBCR_422_I = 0x14,
  RGBA_FP16 = 0x16,
  RAW16 = 0x20,
  BLOB = 0x21,
  IMPLEMENTATION_DEFINED = 0x22,
  YCBCR_420_888 = 0x23,
  RAW_OPAQUE = 0x24,
  RAW10 = 0x25,
  RAW12 = 0x26,
  RGBA_1010102 = 0x2B,
  Y8 = 0x20203859,
  Y16 = 0x20363159,
  YV12 = 0x32315659,
  DEPTH_16 = 0x30,
  DEPTH_24 = 0x31,
  DEPTH_24_STENCIL_8 = 0x32,
  DEPTH_32F = 0x33,
  DEPTH_32F_STENCIL_8 = 0x34,
  STENCIL_8 = 0x35,
  YCBCR_P010 = 0x36,
  HSV_888 = 0x37,
  R_8 = 0x38,
  R_16_UINT = 0x39,
  RG_1616_UINT = 0x3a,
  RGBA_10101010 = 0x3b,
}
