/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.gnss;
@VintfStability
parcelable GnssMeasurement {
  int flags;
  int svid;
  android.hardware.gnss.GnssSignalType signalType;
  double timeOffsetNs;
  int state;
  long receivedSvTimeInNs;
  long receivedSvTimeUncertaintyInNs;
  double antennaCN0DbHz;
  double basebandCN0DbHz;
  double pseudorangeRateMps;
  double pseudorangeRateUncertaintyMps;
  int accumulatedDeltaRangeState;
  double accumulatedDeltaRangeM;
  double accumulatedDeltaRangeUncertaintyM;
  long carrierCycles;
  double carrierPhase;
  double carrierPhaseUncertainty;
  android.hardware.gnss.GnssMultipathIndicator multipathIndicator = android.hardware.gnss.GnssMultipathIndicator.UNKNOWN;
  double snrDb;
  double agcLevelDb;
  double fullInterSignalBiasNs;
  double fullInterSignalBiasUncertaintyNs;
  double satelliteInterSignalBiasNs;
  double satelliteInterSignalBiasUncertaintyNs;
  android.hardware.gnss.SatellitePvt satellitePvt;
  android.hardware.gnss.CorrelationVector[] correlationVectors;
  const int HAS_SNR = 1;
  const int HAS_CARRIER_FREQUENCY = 512;
  const int HAS_CARRIER_CYCLES = 1024;
  const int HAS_CARRIER_PHASE = 2048;
  const int HAS_CARRIER_PHASE_UNCERTAINTY = 4096;
  const int HAS_AUTOMATIC_GAIN_CONTROL = 8192;
  const int HAS_FULL_ISB = 65536;
  const int HAS_FULL_ISB_UNCERTAINTY = 131072;
  const int HAS_SATELLITE_ISB = 262144;
  const int HAS_SATELLITE_ISB_UNCERTAINTY = 524288;
  const int HAS_SATELLITE_PVT = 1048576;
  const int HAS_CORRELATION_VECTOR = 2097152;
  const int STATE_UNKNOWN = 0;
  const int STATE_CODE_LOCK = 1;
  const int STATE_BIT_SYNC = 2;
  const int STATE_SUBFRAME_SYNC = 4;
  const int STATE_TOW_DECODED = 8;
  const int STATE_MSEC_AMBIGUOUS = 16;
  const int STATE_SYMBOL_SYNC = 32;
  const int STATE_GLO_STRING_SYNC = 64;
  const int STATE_GLO_TOD_DECODED = 128;
  const int STATE_BDS_D2_BIT_SYNC = 256;
  const int STATE_BDS_D2_SUBFRAME_SYNC = 512;
  const int STATE_GAL_E1BC_CODE_LOCK = 1024;
  const int STATE_GAL_E1C_2ND_CODE_LOCK = 2048;
  const int STATE_GAL_E1B_PAGE_SYNC = 4096;
  const int STATE_SBAS_SYNC = 8192;
  const int STATE_TOW_KNOWN = 16384;
  const int STATE_GLO_TOD_KNOWN = 32768;
  const int STATE_2ND_CODE_LOCK = 65536;
  const int ADR_STATE_UNKNOWN = 0;
  const int ADR_STATE_VALID = 1;
  const int ADR_STATE_RESET = 2;
  const int ADR_STATE_CYCLE_SLIP = 4;
  const int ADR_STATE_HALF_CYCLE_RESOLVED = 8;
}
