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
@Backing(type="int") @VintfStability
enum FrontendStatusType {
  DEMOD_LOCK = 0,
  SNR = 1,
  BER = 2,
  PER = 3,
  PRE_BER = 4,
  SIGNAL_QUALITY = 5,
  SIGNAL_STRENGTH = 6,
  SYMBOL_RATE = 7,
  FEC = 8,
  MODULATION = 9,
  SPECTRAL = 10,
  LNB_VOLTAGE = 11,
  PLP_ID = 12,
  EWBS = 13,
  AGC = 14,
  LNA = 15,
  LAYER_ERROR = 16,
  MER = 17,
  FREQ_OFFSET = 18,
  HIERARCHY = 19,
  RF_LOCK = 20,
  ATSC3_PLP_INFO = 21,
  MODULATIONS = 22,
  BERS = 23,
  CODERATES = 24,
  BANDWIDTH = 25,
  GUARD_INTERVAL = 26,
  TRANSMISSION_MODE = 27,
  UEC = 28,
  T2_SYSTEM_ID = 29,
  INTERLEAVINGS = 30,
  ISDBT_SEGMENTS = 31,
  TS_DATA_RATES = 32,
  ROLL_OFF = 33,
  IS_MISO = 34,
  IS_LINEAR = 35,
  IS_SHORT_FRAMES = 36,
  ISDBT_MODE = 37,
  ISDBT_PARTIAL_RECEPTION_FLAG = 38,
  STREAM_ID_LIST = 39,
  DVBT_CELL_IDS = 40,
  ATSC3_ALL_PLP_INFO = 41,
  IPTV_CONTENT_URL = 42,
  IPTV_PACKETS_LOST = 43,
  IPTV_PACKETS_RECEIVED = 44,
  IPTV_WORST_JITTER_MS = 45,
  IPTV_AVERAGE_JITTER_MS = 46,
}
