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
interface IGnssConfiguration {
  void setSuplVersion(in int version);
  void setSuplMode(in int mode);
  void setLppProfile(in int lppProfile);
  void setGlonassPositioningProtocol(in int protocol);
  void setEmergencySuplPdn(in boolean enable);
  void setEsExtensionSec(in int emergencyExtensionSeconds);
  void setBlocklist(in android.hardware.gnss.BlocklistedSource[] blocklist);
  const int SUPL_MODE_MSB = 1;
  const int SUPL_MODE_MSA = 2;
  const int LPP_PROFILE_USER_PLANE = 1;
  const int LPP_PROFILE_CONTROL_PLANE = 2;
  const int GLONASS_POS_PROTOCOL_RRC_CPLANE = 1;
  const int GLONASS_POS_PROTOCOL_RRLP_UPLANE = 2;
  const int GLONASS_POS_PROTOCOL_LPP_UPLANE = 4;
}
