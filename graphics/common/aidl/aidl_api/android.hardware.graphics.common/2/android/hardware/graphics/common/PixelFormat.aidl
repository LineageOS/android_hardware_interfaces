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
@Backing(type="int") @VintfStability
enum PixelFormat {
  UNSPECIFIED = 0,
  RGBA_8888 = 1,
  RGBX_8888 = 2,
  RGB_888 = 3,
  RGB_565 = 4,
  BGRA_8888 = 5,
  YCBCR_422_SP = 16,
  YCRCB_420_SP = 17,
  YCBCR_422_I = 20,
  RGBA_FP16 = 22,
  RAW16 = 32,
  BLOB = 33,
  IMPLEMENTATION_DEFINED = 34,
  YCBCR_420_888 = 35,
  RAW_OPAQUE = 36,
  RAW10 = 37,
  RAW12 = 38,
  RGBA_1010102 = 43,
  Y8 = 538982489,
  Y16 = 540422489,
  YV12 = 842094169,
  DEPTH_16 = 48,
  DEPTH_24 = 49,
  DEPTH_24_STENCIL_8 = 50,
  DEPTH_32F = 51,
  DEPTH_32F_STENCIL_8 = 52,
  STENCIL_8 = 53,
  YCBCR_P010 = 54,
  HSV_888 = 55,
}
