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
@Backing(type="int") @VintfStability
enum Dataspace {
  UNKNOWN = 0,
  ARBITRARY = 1,
  STANDARD_SHIFT = 16,
  STANDARD_MASK = 4128768,
  STANDARD_UNSPECIFIED = 0,
  STANDARD_BT709 = 65536,
  STANDARD_BT601_625 = 131072,
  STANDARD_BT601_625_UNADJUSTED = 196608,
  STANDARD_BT601_525 = 262144,
  STANDARD_BT601_525_UNADJUSTED = 327680,
  STANDARD_BT2020 = 393216,
  STANDARD_BT2020_CONSTANT_LUMINANCE = 458752,
  STANDARD_BT470M = 524288,
  STANDARD_FILM = 589824,
  STANDARD_DCI_P3 = 655360,
  STANDARD_ADOBE_RGB = 720896,
  TRANSFER_SHIFT = 22,
  TRANSFER_MASK = 130023424,
  TRANSFER_UNSPECIFIED = 0,
  TRANSFER_LINEAR = 4194304,
  TRANSFER_SRGB = 8388608,
  TRANSFER_SMPTE_170M = 12582912,
  TRANSFER_GAMMA2_2 = 16777216,
  TRANSFER_GAMMA2_6 = 20971520,
  TRANSFER_GAMMA2_8 = 25165824,
  TRANSFER_ST2084 = 29360128,
  TRANSFER_HLG = 33554432,
  RANGE_SHIFT = 27,
  RANGE_MASK = 939524096,
  RANGE_UNSPECIFIED = 0,
  RANGE_FULL = 134217728,
  RANGE_LIMITED = 268435456,
  RANGE_EXTENDED = 402653184,
  SRGB_LINEAR = 138477568,
  SCRGB_LINEAR = 406913024,
  SRGB = 142671872,
  SCRGB = 411107328,
  JFIF = 146931712,
  BT601_625 = 281149440,
  BT601_525 = 281280512,
  BT709 = 281083904,
  DCI_P3_LINEAR = 139067392,
  DCI_P3 = 155844608,
  DISPLAY_P3_LINEAR = 139067392,
  DISPLAY_P3 = 143261696,
  ADOBE_RGB = 151715840,
  BT2020_LINEAR = 138805248,
  BT2020 = 147193856,
  BT2020_PQ = 163971072,
  DEPTH = 4096,
  SENSOR = 4097,
  BT2020_ITU = 281411584,
  BT2020_ITU_PQ = 298188800,
  BT2020_ITU_HLG = 302383104,
  BT2020_HLG = 168165376,
  DISPLAY_BT2020 = 142999552,
  DYNAMIC_DEPTH = 4098,
  JPEG_APP_SEGMENTS = 4099,
  HEIF = 4100,
  BT709_FULL_RANGE = 146866176,
}
