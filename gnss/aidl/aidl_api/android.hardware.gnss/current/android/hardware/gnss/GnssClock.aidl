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
parcelable GnssClock {
  int gnssClockFlags;
  int leapSecond;
  long timeNs;
  double timeUncertaintyNs;
  long fullBiasNs;
  double biasNs;
  double biasUncertaintyNs;
  double driftNsps;
  double driftUncertaintyNsps;
  int hwClockDiscontinuityCount;
  android.hardware.gnss.GnssSignalType referenceSignalTypeForIsb;
  const int HAS_LEAP_SECOND = 1;
  const int HAS_TIME_UNCERTAINTY = 2;
  const int HAS_FULL_BIAS = 4;
  const int HAS_BIAS = 8;
  const int HAS_BIAS_UNCERTAINTY = 16;
  const int HAS_DRIFT = 32;
  const int HAS_DRIFT_UNCERTAINTY = 64;
}
