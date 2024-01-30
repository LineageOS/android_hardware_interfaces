/**
 * Copyright (c) 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
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
enum Dataspace {
  UNKNOWN = 0x0,
  ARBITRARY = 0x1,
  STANDARD_SHIFT = 16,
  STANDARD_MASK = (63 << 16) /* 4128768 */,
  STANDARD_UNSPECIFIED = (0 << 16) /* 0 */,
  STANDARD_BT709 = (1 << 16) /* 65536 */,
  STANDARD_BT601_625 = (2 << 16) /* 131072 */,
  STANDARD_BT601_625_UNADJUSTED = (3 << 16) /* 196608 */,
  STANDARD_BT601_525 = (4 << 16) /* 262144 */,
  STANDARD_BT601_525_UNADJUSTED = (5 << 16) /* 327680 */,
  STANDARD_BT2020 = (6 << 16) /* 393216 */,
  STANDARD_BT2020_CONSTANT_LUMINANCE = (7 << 16) /* 458752 */,
  STANDARD_BT470M = (8 << 16) /* 524288 */,
  STANDARD_FILM = (9 << 16) /* 589824 */,
  STANDARD_DCI_P3 = (10 << 16) /* 655360 */,
  STANDARD_ADOBE_RGB = (11 << 16) /* 720896 */,
  TRANSFER_SHIFT = 22,
  TRANSFER_MASK = (31 << 22) /* 130023424 */,
  TRANSFER_UNSPECIFIED = (0 << 22) /* 0 */,
  TRANSFER_LINEAR = (1 << 22) /* 4194304 */,
  TRANSFER_SRGB = (2 << 22) /* 8388608 */,
  TRANSFER_SMPTE_170M = (3 << 22) /* 12582912 */,
  TRANSFER_GAMMA2_2 = (4 << 22) /* 16777216 */,
  TRANSFER_GAMMA2_6 = (5 << 22) /* 20971520 */,
  TRANSFER_GAMMA2_8 = (6 << 22) /* 25165824 */,
  TRANSFER_ST2084 = (7 << 22) /* 29360128 */,
  TRANSFER_HLG = (8 << 22) /* 33554432 */,
  RANGE_SHIFT = 27,
  RANGE_MASK = (7 << 27) /* 939524096 */,
  RANGE_UNSPECIFIED = (0 << 27) /* 0 */,
  RANGE_FULL = (1 << 27) /* 134217728 */,
  RANGE_LIMITED = (2 << 27) /* 268435456 */,
  RANGE_EXTENDED = (3 << 27) /* 402653184 */,
  SRGB_LINEAR = (((1 << 16) | (1 << 22)) | (1 << 27)) /* 138477568 */,
  SCRGB_LINEAR = (((1 << 16) | (1 << 22)) | (3 << 27)) /* 406913024 */,
  SRGB = (((1 << 16) | (2 << 22)) | (1 << 27)) /* 142671872 */,
  SCRGB = (((1 << 16) | (2 << 22)) | (3 << 27)) /* 411107328 */,
  JFIF = (((2 << 16) | (3 << 22)) | (1 << 27)) /* 146931712 */,
  BT601_625 = (((2 << 16) | (3 << 22)) | (2 << 27)) /* 281149440 */,
  BT601_525 = (((4 << 16) | (3 << 22)) | (2 << 27)) /* 281280512 */,
  BT709 = (((1 << 16) | (3 << 22)) | (2 << 27)) /* 281083904 */,
  DCI_P3_LINEAR = (((10 << 16) | (1 << 22)) | (1 << 27)) /* 139067392 */,
  DCI_P3 = (((10 << 16) | (5 << 22)) | (1 << 27)) /* 155844608 */,
  DISPLAY_P3_LINEAR = (((10 << 16) | (1 << 22)) | (1 << 27)) /* 139067392 */,
  DISPLAY_P3 = (((10 << 16) | (2 << 22)) | (1 << 27)) /* 143261696 */,
  ADOBE_RGB = (((11 << 16) | (4 << 22)) | (1 << 27)) /* 151715840 */,
  ADOBE_RGB_LINEAR = (((11 << 16) | (1 << 22)) | (1 << 27)) /* 139132928 */,
  BT2020_LINEAR = (((6 << 16) | (1 << 22)) | (1 << 27)) /* 138805248 */,
  BT2020 = (((6 << 16) | (3 << 22)) | (1 << 27)) /* 147193856 */,
  BT2020_PQ = (((6 << 16) | (7 << 22)) | (1 << 27)) /* 163971072 */,
  BT2020_LINEAR_EXTENDED = (((6 << 16) | (1 << 22)) | (3 << 27)) /* 407240704 */,
  DEPTH = 0x1000,
  SENSOR = 0x1001,
  BT2020_ITU = (((6 << 16) | (3 << 22)) | (2 << 27)) /* 281411584 */,
  BT2020_ITU_PQ = (((6 << 16) | (7 << 22)) | (2 << 27)) /* 298188800 */,
  BT2020_ITU_HLG = (((6 << 16) | (8 << 22)) | (2 << 27)) /* 302383104 */,
  BT2020_HLG = (((6 << 16) | (8 << 22)) | (1 << 27)) /* 168165376 */,
  DISPLAY_BT2020 = (((6 << 16) | (2 << 22)) | (1 << 27)) /* 142999552 */,
  DYNAMIC_DEPTH = 0x1002,
  JPEG_APP_SEGMENTS = 0x1003,
  HEIF = 0x1004,
  JPEG_R = 0x1005,
  BT709_FULL_RANGE = (((1 << 16) | (3 << 22)) | (1 << 27)) /* 146866176 */,
}
