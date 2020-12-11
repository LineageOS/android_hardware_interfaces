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

package android.hardware.keymint;
@Backing(type="int") @VintfStability
enum TagType {
  INVALID = 0,
  ENUM = 268435456,
  ENUM_REP = 536870912,
  UINT = 805306368,
  UINT_REP = 1073741824,
  ULONG = 1342177280,
  DATE = 1610612736,
  BOOL = 1879048192,
  BIGNUM = -2147483648,
  BYTES = -1879048192,
  ULONG_REP = -1610612736,
}
