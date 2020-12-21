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

package android.hardware.security.keymint;
@RustDerive(Clone=true, Eq=true, Hash=true, Ord=true, PartialEq=true, PartialOrd=true) @VintfStability
union KeyParameterValue {
  int invalid;
  android.hardware.security.keymint.Algorithm algorithm;
  android.hardware.security.keymint.BlockMode blockMode;
  android.hardware.security.keymint.PaddingMode paddingMode;
  android.hardware.security.keymint.Digest digest;
  android.hardware.security.keymint.EcCurve ecCurve;
  android.hardware.security.keymint.KeyOrigin origin;
  android.hardware.security.keymint.KeyPurpose keyPurpose;
  android.hardware.security.keymint.HardwareAuthenticatorType hardwareAuthenticatorType;
  android.hardware.security.keymint.SecurityLevel securityLevel;
  boolean boolValue;
  int integer;
  long longInteger;
  long dateTime;
  byte[] blob;
}
