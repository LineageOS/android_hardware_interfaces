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
parcelable GnssSignalType {
  android.hardware.gnss.GnssConstellationType constellation;
  double carrierFrequencyHz;
  @utf8InCpp String codeType;
  const @utf8InCpp String CODE_TYPE_A = "A";
  const @utf8InCpp String CODE_TYPE_B = "B";
  const @utf8InCpp String CODE_TYPE_C = "C";
  const @utf8InCpp String CODE_TYPE_D = "D";
  const @utf8InCpp String CODE_TYPE_I = "I";
  const @utf8InCpp String CODE_TYPE_L = "L";
  const @utf8InCpp String CODE_TYPE_M = "M";
  const @utf8InCpp String CODE_TYPE_N = "N";
  const @utf8InCpp String CODE_TYPE_P = "P";
  const @utf8InCpp String CODE_TYPE_Q = "Q";
  const @utf8InCpp String CODE_TYPE_S = "S";
  const @utf8InCpp String CODE_TYPE_W = "W";
  const @utf8InCpp String CODE_TYPE_X = "X";
  const @utf8InCpp String CODE_TYPE_Y = "Y";
  const @utf8InCpp String CODE_TYPE_Z = "Z";
  const @utf8InCpp String CODE_TYPE_UNKNOWN = "UNKNOWN";
}
