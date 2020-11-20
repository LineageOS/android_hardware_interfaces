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
