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
  DEMOD_LOCK,
  SNR,
  BER,
  PER,
  PRE_BER,
  SIGNAL_QUALITY,
  SIGNAL_STRENGTH,
  SYMBOL_RATE,
  FEC,
  MODULATION,
  SPECTRAL,
  LNB_VOLTAGE,
  PLP_ID,
  EWBS,
  AGC,
  LNA,
  LAYER_ERROR,
  MER,
  FREQ_OFFSET,
  HIERARCHY,
  RF_LOCK,
  ATSC3_PLP_INFO,
  MODULATIONS,
  BERS,
  CODERATES,
  BANDWIDTH,
  GUARD_INTERVAL,
  TRANSMISSION_MODE,
  UEC,
  T2_SYSTEM_ID,
  INTERLEAVINGS,
  ISDBT_SEGMENTS,
  TS_DATA_RATES,
  ROLL_OFF,
  IS_MISO,
  IS_LINEAR,
  IS_SHORT_FRAMES,
  ISDBT_MODE,
  ISDBT_PARTIAL_RECEPTION_FLAG,
  STREAM_ID_LIST,
  DVBT_CELL_IDS,
  ATSC3_ALL_PLP_INFO,
  IPTV_CONTENT_URL,
  IPTV_PACKETS_LOST,
  IPTV_PACKETS_RECEIVED,
  IPTV_WORST_JITTER_MS,
  IPTV_AVERAGE_JITTER_MS,
}
