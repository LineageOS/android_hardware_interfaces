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

package android.hardware.security.keymint;
@VintfStability
interface IKeyMintOperation {
  int update(in @nullable android.hardware.security.keymint.KeyParameterArray inParams, in @nullable byte[] input, in @nullable android.hardware.security.keymint.HardwareAuthToken inAuthToken, in @nullable android.hardware.security.secureclock.TimeStampToken inTimeStampToken, out @nullable android.hardware.security.keymint.KeyParameterArray outParams, out @nullable android.hardware.security.keymint.ByteArray output);
  byte[] finish(in @nullable android.hardware.security.keymint.KeyParameterArray inParams, in @nullable byte[] input, in @nullable byte[] inSignature, in @nullable android.hardware.security.keymint.HardwareAuthToken authToken, in @nullable android.hardware.security.secureclock.TimeStampToken inTimeStampToken, out @nullable android.hardware.security.keymint.KeyParameterArray outParams);
  void abort();
}
