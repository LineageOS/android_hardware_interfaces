/*
 * Copyright 2021 The Android Open Source Project
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

package android.hardware.tv.tuner;
/* @hide */
@Backing(type="long") @VintfStability
enum FrontendInnerFec {
  FEC_UNDEFINED = 0,
  AUTO = 1,
  FEC_1_2 = 2,
  FEC_1_3 = 4,
  FEC_1_4 = 8,
  FEC_1_5 = 16,
  FEC_2_3 = 32,
  FEC_2_5 = 64,
  FEC_2_9 = 128,
  FEC_3_4 = 256,
  FEC_3_5 = 512,
  FEC_4_5 = 1024,
  FEC_4_15 = 2048,
  FEC_5_6 = 4096,
  FEC_5_9 = 8192,
  FEC_6_7 = 16384,
  FEC_7_8 = 32768,
  FEC_7_9 = 65536,
  FEC_7_15 = 131072,
  FEC_8_9 = 262144,
  FEC_8_15 = 524288,
  FEC_9_10 = 1048576,
  FEC_9_20 = 2097152,
  FEC_11_15 = 4194304,
  FEC_11_20 = 8388608,
  FEC_11_45 = 16777216,
  FEC_13_18 = 33554432,
  FEC_13_45 = 67108864,
  FEC_14_45 = 134217728,
  FEC_23_36 = 268435456,
  FEC_25_36 = 536870912,
  FEC_26_45 = 1073741824,
  FEC_28_45 = 2147483648,
  FEC_29_45 = 4294967296,
  FEC_31_45 = 8589934592,
  FEC_32_45 = 17179869184,
  FEC_77_90 = 34359738368,
  FEC_2_15 = 68719476736,
  FEC_3_15 = 137438953472,
  FEC_5_15 = 274877906944,
  FEC_6_15 = 549755813888,
  FEC_9_15 = 1099511627776,
  FEC_10_15 = 2199023255552,
  FEC_12_15 = 4398046511104,
  FEC_13_15 = 8796093022208,
  FEC_18_30 = 17592186044416,
  FEC_20_30 = 35184372088832,
  FEC_90_180 = 70368744177664,
  FEC_96_180 = 140737488355328,
  FEC_104_180 = 281474976710656,
  FEC_128_180 = 562949953421312,
  FEC_132_180 = 1125899906842624,
  FEC_135_180 = 2251799813685248,
  FEC_140_180 = 4503599627370496,
}
