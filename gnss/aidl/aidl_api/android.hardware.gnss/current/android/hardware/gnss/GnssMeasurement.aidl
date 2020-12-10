///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL interface (or parcelable). Do not try to
// edit this file. It looks like you are doing that because you have modified
// an AIDL interface in a backward-incompatible way, e.g., deleting a function
// from an interface or a field from a parcelable and it broke the build. That
// breakage is intended.
//
// You must not make a backward incompatible changes to the AIDL files built
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
  float carrierFrequencyHz;
  long carrierCycles;
  double carrierPhase;
  double carrierPhaseUncertainty;
  android.hardware.gnss.GnssMultipathIndicator multipathIndicator;
  double snrDb;
  double agcLevelDb;
  double fullInterSignalBiasNs;
  double fullInterSignalBiasUncertaintyNs;
  double satelliteInterSignalBiasNs;
  double satelliteInterSignalBiasUncertaintyNs;
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
